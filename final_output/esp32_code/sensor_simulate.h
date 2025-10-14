/*
 * Sensor Simulation Module
 * 
 * Generates random sensor data within realistic ranges for weather prediction testing
 * Handles:
 * - Random sensor value generation (Temperature, Humidity, Pressure, Light, Gas)
 * - 1-second sampling interval
 * - 15-second averaging for predictions (15 samples)
 * - ML model prediction using averaged data
 * - Cloud upload (ThingSpeak) with all metrics
 * - Continuous operation until stopped by user command
 * 
 * Features:
 * - Weather-pattern-based sensor generation (cyclic: all 5 classes equally, 30s each)
 * - UPDATED for OPTION 3 HYBRID BALANCED model (simplified thresholds)
 * - Realistic sensor value ranges from training data
 * - Automatic feature scaling for ML model
 * - Prediction accuracy tracking
 * - Cloud integration with WiFi status monitoring (ThingSpeak + Firebase)
 * - Complete data logging
 * 
 * ⚡ NEW MODEL THRESHOLDS (Option 3 - Hybrid Balanced):
 * ────────────────────────────────────────────────────────
 * Priority 1: SUNNY  -> lux > 130 (SIMPLIFIED - no other conditions!)
 * Priority 2: STORMY -> pressure < 97200 Pa
 * Priority 3: FOGGY  -> humidity > 48% AND lux < 120
 * Priority 4: RAINY  -> pressure < 98000 Pa AND humidity > 42%
 * Default:    CLOUDY -> Everything else (middle ranges)
 * 
 * Expected Prediction Distribution (after model retraining):
 * - Sunny:  ~20-25% (bright conditions - lux >130)
 * - Cloudy: ~30-35% (normal indoor - middle ranges)
 * - Rainy:  ~15-20% (low pressure + humidity)
 * - Stormy: ~12-15% (very low pressure)
 * - Foggy:  ~10-12% (humid + dark)
 * 
 * Commands:
 * - startsim: Begin continuous simulation
 * - Any key: Stop simulation
 * 
 * ⚠️  EXPECTED MODEL BEHAVIOR (Option 3 - Hybrid Balanced):
 * With the NEW simplified thresholds, the model should have HIGH accuracy.
 * 
 * 1. Clear Separation (NEW thresholds reduce overlap):
 *    ✅ Sunny: lux >130 (VERY CLEAR - first priority check)
 *    ✅ Stormy: pressure <97200 (DISTINCTIVE - second priority)
 *    ✅ Foggy: humidity >48 AND lux <120 (CLEAR combination)
 *    ✅ Rainy: pressure <98000 AND humidity >42 (CLEAR combination)
 *    ✅ Cloudy: Everything else (DEFAULT - middle ranges)
 * 
 * 2. Expected Accuracy:
 *    • Overall: ~75-85% (realistic for indoor sensor data)
 *    • Sunny: HIGH accuracy (~85-90%) - very distinctive lux threshold
 *    • Stormy: HIGH accuracy (~80-85%) - distinctive pressure
 *    • Foggy/Rainy: GOOD accuracy (~70-80%) - some overlap possible
 *    • Cloudy: GOOD accuracy (~75-80%) - catches everything else
 * 
 * 3. Simulation Testing:
 *    ✅ Each pattern sustained for 30 seconds
 *    ✅ Cycling through all 5 classes (0→1→2→3→4→0)
 *    ✅ Predictions every 15 seconds (15 samples averaged)
 *    ✅ Should see ALL 5 CLASSES appear (no "always Cloudy" anymore!)
 * 
 * Examples of ACCEPTABLE behavior:
 *    ✓ Sunny correctly predicted when lux >200
 *    ✓ Stormy correctly predicted when pressure <97000
 *    ✓ Occasional Rainy↔Stormy confusion (both low pressure)
 *    ✓ Occasional Foggy↔Cloudy confusion (boundary cases)
 * 
 * Examples of PROBLEMS (would indicate issues):
 *    ✗ Sunny (lux 500) predicted as Cloudy - scaling/threshold bug
 *    ✗ Stormy (pressure 96500) predicted as Sunny - priority bug
 *    ✗ All predictions still Cloudy - model not retrained
 *    ✗ >50% misclassification - wrong model loaded
 */

#ifndef SENSOR_SIMULATE_H
#define SENSOR_SIMULATE_H

#include <Arduino.h>
#include "weather_model_250.h"
#include "weather_scaling.h"
#include <HTTPClient.h>
#include <WiFi.h>

// ThingSpeak Configuration
#define THINGSPEAK_CHANNEL_ID "3108323"
#define THINGSPEAK_API_KEY "J3GFLQKI0TVR6JC2"
#define THINGSPEAK_SERVER "http://api.thingspeak.com"

class SensorSimulator {
private:
    // External managers
    class FirebaseManager* firebaseManager;
    
    // Timing constants
    static const unsigned long SENSOR_INTERVAL = 1000;      // 1 second
    static const unsigned long PREDICTION_INTERVAL = 15000; // 15 seconds (ThingSpeak rate limit)
    static const int BUFFER_SIZE = 15;                      // 15 readings for averaging
    
    // Sensor value ranges - MATCHED TO TRAINING DATA
    // These ranges MUST match the scaling parameters in weather_scaling.h
    // Using training data ranges ensures model sees familiar input patterns
    static constexpr float TEMP_MIN = 19.0f;      // Match training: 19.0°C
    static constexpr float TEMP_MAX = 30.0f;      // Match training: 30.0°C
    static constexpr float HUMID_MIN = 29.3f;     // Match training: 29.3%
    static constexpr float HUMID_MAX = 56.9f;     // Match training: 56.9%
    static constexpr float PRESSURE_MIN = 96352.7f;  // Match training: 96352.68 Pa
    static constexpr float PRESSURE_MAX = 100301.1f; // Match training: 100301.06 Pa
    static constexpr float LUX_MIN = 0.0f;        // Match training: 0.0 lux
    static constexpr float LUX_MAX = 632.1f;      // Match training: 632.08 lux
    static constexpr float GAS_MIN = 50.0f;       // Keep: baseline
    static constexpr float GAS_MAX = 2000.0f;     // Keep: max
    
    // Sensor data buffers for averaging
    float tempBuffer[BUFFER_SIZE];
    float humidBuffer[BUFFER_SIZE];
    float pressureBuffer[BUFFER_SIZE];
    float luxBuffer[BUFFER_SIZE];
    float gasBuffer[BUFFER_SIZE];
    int bufferIndex;
    
    // Current sensor readings
    float currentTemp;
    float currentHumid;
    float currentPressure;
    float currentLux;
    float currentGas;
    
    // Timing
    unsigned long lastSensorRead;
    unsigned long lastPrediction;
    unsigned long simulationStartTime;
    
    // Statistics
    unsigned long totalReadings;
    unsigned long totalPredictions;
    unsigned long totalCloudUploads;
    unsigned long successfulUploads;
    unsigned long failedUploads;
    
    // Prediction tracking
    int predictionCounts[5];  // Count of each weather class
    const char* weatherClasses[5] = {"Cloudy", "Foggy", "Rainy", "Stormy", "Sunny"};
    const char* weatherEmojis[5] = {"☁️", "🌫️", "🌧️", "⛈️", "☀️"};
    
    // State
    bool isRunning;
    bool wifiAvailable;
    
    // Sustained weather pattern control
    int currentWeatherPattern;       // Current weather pattern (0-4)
    unsigned long patternStartTime;  // When current pattern started
    static const unsigned long PATTERN_DURATION = 30000; // 30 seconds per pattern
    
    // ML Classifier
    Eloquent::ML::Port::RandomForest classifier;
    
    // Cloud integration
    HTTPClient http;
    
public:
    SensorSimulator() {
        firebaseManager = nullptr;
        bufferIndex = 0;
        lastSensorRead = 0;
        lastPrediction = 0;
        simulationStartTime = 0;
        totalReadings = 0;
        totalPredictions = 0;
        totalCloudUploads = 0;
        successfulUploads = 0;
        failedUploads = 0;
        isRunning = false;
        wifiAvailable = false;
        currentWeatherPattern = -1;  // Will be set on first reading
        patternStartTime = 0;
        
        // Initialize prediction counts
        for (int i = 0; i < 5; i++) {
            predictionCounts[i] = 0;
        }
        
        // Initialize buffers
        for (int i = 0; i < BUFFER_SIZE; i++) {
            tempBuffer[i] = 0.0f;
            humidBuffer[i] = 0.0f;
            pressureBuffer[i] = 0.0f;
            luxBuffer[i] = 0.0f;
            gasBuffer[i] = 0.0f;
        }
    }
    
    // Initialize simulator
    void begin() {
        Serial.println("╔════════════════════════════════════════════════════════╗");
        Serial.println("║        SENSOR SIMULATOR INITIALIZED                    ║");
        Serial.println("╚════════════════════════════════════════════════════════╝");
        Serial.println();
        Serial.println("📊 Simulation Configuration:");
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.println("   Sensor Reading:  Every 1 second");
        Serial.println("   Prediction:      Every 15 seconds (15 samples averaged)");
        Serial.println("   Cloud Upload:    After each prediction");
        Serial.println("   Rate Limit:      Complies with ThingSpeak (15s minimum)");
        Serial.println();
        Serial.println("📈 Sensor Value Ranges (from training data):");
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("   Temperature: %.1f - %.1f °C\n", TEMP_MIN, TEMP_MAX);
        Serial.printf("   Humidity:    %.1f - %.1f %%\n", HUMID_MIN, HUMID_MAX);
        Serial.printf("   Pressure:    %.0f - %.0f Pa\n", PRESSURE_MIN, PRESSURE_MAX);
        Serial.printf("   Light (Lux): %.0f - %.0f lux\n", LUX_MIN, LUX_MAX);
        Serial.printf("   Gas (PPM):   %.0f - %.0f ppm\n", GAS_MIN, GAS_MAX);
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.println();
        Serial.println("🎯 Weather Classes:");
        Serial.println("   0: Cloudy ☁️   1: Foggy 🌫️   2: Rainy 🌧️");
        Serial.println("   3: Stormy ⛈️   4: Sunny ☀️");
        Serial.println();
        Serial.println("ℹ️  Note: Occasional misclassifications (Cloudy↔Sunny,");
        Serial.println("   Rainy↔Stormy) are normal within sensor noise ranges.");
        Serial.println();
    }
    
    // Set WiFi availability
    void setWiFiStatus(bool available) {
        wifiAvailable = available;
    }
    
    // Set Firebase manager
    void setFirebaseManager(class FirebaseManager* fbManager) {
        firebaseManager = fbManager;
    }
    
    // Start simulation
    void start() {
        if (isRunning) {
            Serial.println("⚠️  Simulation already running!");
            return;
        }
        
        Serial.println("╔════════════════════════════════════════════════════════╗");
        Serial.println("║           🚀 STARTING SIMULATION                       ║");
        Serial.println("╚════════════════════════════════════════════════════════╝");
        Serial.println();
        Serial.println("🔄 Simulation Mode: CONTINUOUS");
        Serial.println("   • Sensor readings every 1 second");
        Serial.println("   • Predictions every 15 seconds (15 samples averaged)");
        Serial.println("   • Cloud uploads after each prediction (ThingSpeak rate limit compliant)");
        Serial.println();
        Serial.println("⏹️  Press ANY KEY to stop simulation");
        Serial.println();
        Serial.println("═══════════════════════════════════════════════════════════");
        Serial.println();
        
        isRunning = true;
        simulationStartTime = millis();
        bufferIndex = 0;
        lastSensorRead = 0;
        lastPrediction = 0;
        currentWeatherPattern = -1;  // Will trigger first pattern selection
        patternStartTime = 0;
        
        // Reset statistics
        totalReadings = 0;
        totalPredictions = 0;
        for (int i = 0; i < 5; i++) {
            predictionCounts[i] = 0;
        }
    }
    
    // Stop simulation
    void stop() {
        if (!isRunning) {
            return;
        }
        
        isRunning = false;
        unsigned long totalTime = (millis() - simulationStartTime) / 1000;
        
        Serial.println();
        Serial.println("═══════════════════════════════════════════════════════════");
        Serial.println("╔════════════════════════════════════════════════════════╗");
        Serial.println("║           🛑 SIMULATION STOPPED                        ║");
        Serial.println("╚════════════════════════════════════════════════════════╝");
        Serial.println();
        Serial.println("📊 Simulation Statistics:");
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("   Runtime:         %lu seconds\n", totalTime);
        Serial.printf("   Sensor Readings: %lu\n", totalReadings);
        Serial.printf("   Predictions:     %lu\n", totalPredictions);
        Serial.printf("   Cloud Uploads:   %lu (✅ %lu, ❌ %lu)\n", 
                     totalCloudUploads, successfulUploads, failedUploads);
        
        if (totalCloudUploads > 0) {
            float successRate = (successfulUploads * 100.0f) / totalCloudUploads;
            Serial.printf("   Upload Success:  %.1f%%\n", successRate);
        }
        
        Serial.println();
        Serial.println("🌤️  Weather Prediction Distribution:");
        Serial.println("─────────────────────────────────────────────────────────");
        
        for (int i = 0; i < 5; i++) {
            if (totalPredictions > 0) {
                float percentage = (predictionCounts[i] * 100.0f) / totalPredictions;
                Serial.printf("   %s %-8s: %3lu predictions (%.1f%%)\n", 
                             weatherEmojis[i], weatherClasses[i], 
                             predictionCounts[i], percentage);
            }
        }
        
        Serial.println("═══════════════════════════════════════════════════════════");
        Serial.println();
        Serial.println("Type 'startsim' to run again, or 'help' for commands");
        Serial.println();
    }
    
    // Update simulation (call in loop)
    void update() {
        if (!isRunning) {
            return;
        }
        
        unsigned long currentTime = millis();
        
        // Read sensors every 1 second
        if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
            lastSensorRead = currentTime;
            readSensors();
        }
        
        // Make prediction every 5 seconds
        if (currentTime - lastPrediction >= PREDICTION_INTERVAL) {
            lastPrediction = currentTime;
            makePrediction();
        }
    }
    
    // Check if simulation is running
    bool running() {
        return isRunning;
    }
    
private:
    // Generate random sensor values with sustained weather patterns
    void readSensors() {
        unsigned long currentTime = millis();
        
        // Check if we need to switch to a new weather pattern
        if (currentWeatherPattern == -1 || 
            (currentTime - patternStartTime) >= PATTERN_DURATION) {
            
            // Select new weather pattern (cycle through all 5 for demo diversity)
            if (currentWeatherPattern == -1) {
                currentWeatherPattern = 0; // Start with Cloudy
            } else {
                currentWeatherPattern = (currentWeatherPattern + 1) % 5; // Cycle: 0→1→2→3→4→0
            }
            
            patternStartTime = currentTime;
            
            // Announce pattern change
            Serial.println();
            Serial.printf("🔄 Weather Pattern Changed → %s %s (sustained for 30s)\n", 
                         weatherEmojis[currentWeatherPattern], 
                         weatherClasses[currentWeatherPattern]);
            Serial.println();
        }
        
        // Generate sensor values based on SUSTAINED weather pattern
        // UPDATED to match OPTION 3 HYBRID BALANCED model thresholds!
        // NEW LOGIC (from notebook classify_weather function):
        //   Priority 1: SUNNY  -> lux > 130
        //   Priority 2: STORMY -> pressure < 97200
        //   Priority 3: FOGGY  -> humidity > 48 AND lux < 120
        //   Priority 4: RAINY  -> pressure < 98000 AND humidity > 42
        //   Default:    CLOUDY -> Everything else
        
        if (currentWeatherPattern == 0) {
            // Cloudy (Class 0) - Normal indoor conditions (DEFAULT case)
            // MUST NOT trigger other conditions:
            //   - lux MUST be 130 or BELOW (not Sunny)
            //   - pressure MUST be 97200 or ABOVE (not Stormy)
            //   - If humidity >48, lux MUST be 120 or ABOVE (not Foggy)
            //   - If pressure <98000, humidity MUST be 42 or BELOW (not Rainy)
            currentLux = randomFloat(60.0f, 130.0f);        // BELOW Sunny threshold (130)
            currentPressure = randomFloat(98000.0f, 99500.0f); // ABOVE Rainy/Stormy threshold
            currentHumid = randomFloat(38.0f, 48.0f);       // BELOW Foggy/Rainy threshold
            currentTemp = randomFloat(22.0f, 26.0f);        // Mid-range temp
            currentGas = randomFloat(200.0f, 600.0f);
            
        } else if (currentWeatherPattern == 1) {
            // Foggy (Class 1) - High humidity + low light
            // MUST MATCH: humidity > 48 AND lux < 120
            // MUST AVOID: lux > 130 (Sunny), pressure < 97200 (Stormy)
            currentHumid = randomFloat(48.1f, 56.9f);       // ABOVE 48 threshold
            currentLux = randomFloat(0.0f, 119.0f);         // BELOW 120 threshold
            currentPressure = randomFloat(97300.0f, 99000.0f); // ABOVE Stormy threshold
            currentTemp = randomFloat(20.0f, 24.0f);        // Cooler side
            currentGas = randomFloat(400.0f, 800.0f);
            
        } else if (currentWeatherPattern == 2) {
            // Rainy (Class 2) - Low pressure + high humidity
            // MUST MATCH: pressure < 98000 AND humidity > 42
            // MUST AVOID: lux > 130 (Sunny), pressure < 97200 (Stormy)
            // MUST AVOID: humidity > 48 AND lux < 120 (Foggy priority)
            currentPressure = randomFloat(97200.0f, 97999.0f); // Between Stormy and threshold
            currentHumid = randomFloat(42.1f, 52.0f);       // ABOVE 42 threshold
            currentLux = randomFloat(30.0f, 130.0f);        // Keep moderate (avoid Foggy)
            currentTemp = randomFloat(19.0f, 23.0f);        // Cooler (near min)
            currentGas = randomFloat(300.0f, 700.0f);
            
        } else if (currentWeatherPattern == 3) {
            // Stormy (Class 3) - Very low pressure
            // MUST MATCH: pressure < 97200 (Priority 2 - checked early!)
            // MUST AVOID: lux > 130 (Sunny has Priority 1)
            currentPressure = randomFloat(96352.7f, 97199.0f); // BELOW 97200 threshold
            currentHumid = randomFloat(45.0f, 56.5f);       // High humidity (storm conditions)
            currentLux = randomFloat(0.0f, 100.0f);         // Very dark (stormy)
            currentTemp = randomFloat(19.5f, 23.0f);        // Cool temperature
            currentGas = randomFloat(350.0f, 900.0f);
            
        } else {
            // Sunny (Class 4) - Bright light (Priority 1 - checked FIRST!)
            // MUST MATCH: lux > 130 (SIMPLIFIED - no other conditions!)
            // This has HIGHEST priority, so ANY lux >130 = Sunny
            currentLux = randomFloat(131.0f, 632.1f);       // ABOVE 130 threshold
            currentPressure = randomFloat(98500.0f, 100301.1f); // High pressure (typical sunny)
            currentHumid = randomFloat(29.3f, 42.0f);       // Low humidity (typical sunny)
            currentTemp = randomFloat(25.0f, 30.0f);        // Warmer (near max)
            currentGas = randomFloat(100.0f, 400.0f);
        }
        
        // Store in buffer
        tempBuffer[bufferIndex] = currentTemp;
        humidBuffer[bufferIndex] = currentHumid;
        pressureBuffer[bufferIndex] = currentPressure;
        luxBuffer[bufferIndex] = currentLux;
        gasBuffer[bufferIndex] = currentGas;
        
        bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
        totalReadings++;
        
        // Display reading
        unsigned long elapsed = (millis() - simulationStartTime) / 1000;
        Serial.printf("[%02lu:%02lu] Reading #%lu: ", elapsed/60, elapsed%60, totalReadings);
        Serial.printf("🌡️ %.1f°C | 💧 %.1f%% | 🌀 %.0fPa | 💡 %.0flux | 🌫️ %.0fppm\n",
                     currentTemp, currentHumid, currentPressure, currentLux, currentGas);
    }
    
    // Make prediction using averaged data
    void makePrediction() {
        // Calculate averages from buffer
        float avgTemp = 0, avgHumid = 0, avgPressure = 0, avgLux = 0, avgGas = 0;
        
        for (int i = 0; i < BUFFER_SIZE; i++) {
            avgTemp += tempBuffer[i];
            avgHumid += humidBuffer[i];
            avgPressure += pressureBuffer[i];
            avgLux += luxBuffer[i];
            avgGas += gasBuffer[i];
        }
        
        avgTemp /= BUFFER_SIZE;
        avgHumid /= BUFFER_SIZE;
        avgPressure /= BUFFER_SIZE;
        avgLux /= BUFFER_SIZE;
        avgGas /= BUFFER_SIZE;
        
        // Display prediction header
        Serial.println();
        Serial.println("═══════════════════════════════════════════════════════════");
        Serial.println("🔮 MAKING PREDICTION (15-second averaged data - 15 samples)");
        Serial.println("═══════════════════════════════════════════════════════════");
        Serial.println("📊 Averaged Sensor Data:");
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("   🌡️  Temperature: %.2f °C\n", avgTemp);
        Serial.printf("   💧 Humidity:    %.2f %%\n", avgHumid);
        Serial.printf("   🌀 Pressure:    %.2f Pa (%.2f hPa)\n", avgPressure, avgPressure/100.0f);
        Serial.printf("   💡 Light (Lux): %.2f lux (%s)\n", avgLux, getLightCondition(avgLux));
        Serial.printf("   🌫️  Gas (PPM):   %.2f ppm (%s)\n", avgGas, getAirQuality(avgGas));
        Serial.println("─────────────────────────────────────────────────────────");
        
        // Scale features for ML model
        float scaledFeatures[4];
        scaledFeatures[0] = scale_temperature(avgTemp);
        scaledFeatures[1] = scale_humidity(avgHumid);
        scaledFeatures[2] = scale_pressure(avgPressure);
        scaledFeatures[3] = scale_lux(avgLux);
        
        // Make prediction and measure time
        unsigned long startTime = micros();
        int predictedClass = classifier.predict(scaledFeatures);
        unsigned long endTime = micros();
        unsigned long inferenceTime = endTime - startTime;
        
        // Update statistics
        totalPredictions++;
        predictionCounts[predictedClass]++;
        
        // Display prediction result
        Serial.println();
        Serial.println("🎯 Prediction Result:");
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("   Weather:    %s %s\n", 
                     weatherEmojis[predictedClass], 
                     weatherClasses[predictedClass]);
        Serial.printf("   Class ID:   %d\n", predictedClass);
        Serial.printf("   Inference:  %lu µs (%.3f ms)\n", 
                     inferenceTime, inferenceTime/1000.0f);
        Serial.printf("   Prediction: #%lu\n", totalPredictions);
        Serial.println("─────────────────────────────────────────────────────────");
        
        // Upload to cloud
        if (wifiAvailable) {
            uploadToCloud(avgTemp, avgHumid, avgPressure, avgLux, avgGas, 
                         predictedClass, inferenceTime);
            
            // Backup to Firebase (if configured)
            if (firebaseManager != nullptr) {
                firebaseManager->backupData(avgTemp, avgHumid, avgPressure, avgLux,
                                           weatherClasses[predictedClass], inferenceTime);
            }
        } else {
            Serial.println();
            Serial.println("⚠️  Cloud Upload: SKIPPED (WiFi not connected)");
        }
        
        Serial.println("═══════════════════════════════════════════════════════════");
        Serial.println();
    }
    
    // Upload data to ThingSpeak
    void uploadToCloud(float temp, float humid, float pressure, float lux, float gas,
                       int prediction, unsigned long inferenceTime) {
        Serial.println();
        Serial.println("☁️  Uploading to ThingSpeak...");
        Serial.println("─────────────────────────────────────────────────────────");
        
        totalCloudUploads++;
        
        // CRITICAL: Validate WiFi connection state before upload
        if (WiFi.status() != WL_CONNECTED) {
            failedUploads++;
            Serial.println("   Status:   ❌ WiFi NOT CONNECTED");
            Serial.println("   Error:    WiFi connection lost during simulation");
            Serial.println("─────────────────────────────────────────────────────────");
            return;
        }
        
        // CRITICAL: Validate DNS resolution before upload (ESP32 DNS can fail intermittently)
        Serial.println("   🔍 Validating connection...");
        IPAddress serverIP;
        if (!WiFi.hostByName("api.thingspeak.com", serverIP)) {
            failedUploads++;
            Serial.println("   Status:   ❌ DNS RESOLUTION FAILED");
            Serial.println("   Error:    Cannot resolve api.thingspeak.com");
            Serial.println("   Hint:     Router may have lost internet connection");
            Serial.println("─────────────────────────────────────────────────────────");
            delay(2000);  // Wait before next attempt
            return;
        }
        Serial.printf("   ✅ DNS OK: %s\n", serverIP.toString().c_str());
        
        // Build ThingSpeak URL with all fields
        String url = String(THINGSPEAK_SERVER) + "/update?api_key=" + THINGSPEAK_API_KEY;
        url += "&field1=" + String(temp, 2);           // Temperature
        url += "&field2=" + String(humid, 2);          // Humidity
        url += "&field3=" + String(pressure, 2);       // Pressure
        url += "&field4=" + String(lux, 2);            // Lux
        url += "&field5=" + String(gas, 2);            // Gas PPM
        url += "&field6=" + String(prediction);        // Prediction class
        url += "&field7=" + String(inferenceTime);     // Inference time
        url += "&field8=" + String(WiFi.RSSI());       // WiFi signal
        
        Serial.println("   Target:   api.thingspeak.com");
        Serial.printf("   Channel:  %s\n", THINGSPEAK_CHANNEL_ID);
        Serial.println("   Data:");
        Serial.printf("      Field 1 (Temp):      %.2f °C\n", temp);
        Serial.printf("      Field 2 (Humid):     %.2f %%\n", humid);
        Serial.printf("      Field 3 (Pressure):  %.2f Pa\n", pressure);
        Serial.printf("      Field 4 (Lux):       %.2f lux\n", lux);
        Serial.printf("      Field 5 (Gas):       %.2f ppm\n", gas);
        Serial.printf("      Field 6 (Predict):   %d (%s)\n", prediction, weatherClasses[prediction]);
        Serial.printf("      Field 7 (Inference): %lu µs\n", inferenceTime);
        Serial.printf("      Field 8 (WiFi RSSI): %d dBm\n", WiFi.RSSI());
        
        // Make HTTP request with proper configuration
        http.begin(url);
        http.setReuse(false);  // CRITICAL: Disable connection reuse to prevent ESP32 HTTP issues
        http.setTimeout(5000);
        int httpCode = http.GET();
        
        if (httpCode > 0) {
            if (httpCode == 200) {
                String response = http.getString();
                successfulUploads++;
                Serial.println("   Status:   ✅ SUCCESS");
                Serial.printf("   Response: Entry #%s created\n", response.c_str());
                Serial.printf("   Stats:    %lu/%lu successful (%.1f%%)\n", 
                             successfulUploads, totalCloudUploads,
                             (successfulUploads * 100.0f) / totalCloudUploads);
            } else {
                failedUploads++;
                Serial.printf("   Status:   ❌ HTTP %d\n", httpCode);
            }
        } else {
            failedUploads++;
            Serial.println("   Status:   ❌ CONNECTION FAILED");
            Serial.printf("   Error:    %s\n", http.errorToString(httpCode).c_str());
            Serial.println("   Possible causes:");
            Serial.println("      • HTTP client not fully released from previous request");
            Serial.println("      • Router lost internet connection");
            Serial.println("      • ThingSpeak server temporarily unavailable");
        }
        
        http.end();
        Serial.println("─────────────────────────────────────────────────────────");
    }
    
    // Generate random float in range
    float randomFloat(float min, float max) {
        return min + (random(0, 10000) / 10000.0f) * (max - min);
    }
    
    // Get light condition description
    const char* getLightCondition(float lux) {
        if (lux < 10) return "Dark";
        else if (lux < 50) return "Dim";
        else if (lux < 200) return "Indoor";
        else if (lux < 400) return "Overcast";
        else return "Bright";
    }
    
    // Get air quality description
    const char* getAirQuality(float ppm) {
        if (ppm < 200) return "Excellent";
        else if (ppm < 400) return "Good";
        else if (ppm < 700) return "Fair";
        else if (ppm < 1000) return "Poor";
        else return "Hazardous";
    }
};

#endif // SENSOR_SIMULATE_H
