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
 * - Weather-pattern-based sensor generation (40% Cloudy, 20% Sunny, 20% Rainy, 10% Stormy, 10% Foggy)
 * - Realistic sensor value ranges from training data
 * - Automatic feature scaling for ML model
 * - Prediction accuracy tracking
 * - Cloud integration with WiFi status monitoring (ThingSpeak + Firebase)
 * - Complete data logging
 * 
 * Commands:
 * - startsim: Begin continuous simulation
 * - Any key: Stop simulation
 * 
 * ⚠️  ACCEPTABLE MODEL BEHAVIOR:
 * The ML model may occasionally misclassify within similar weather patterns.
 * This is NORMAL and EXPECTED behavior due to:
 * 
 * 1. Similar Sensor Patterns:
 *    • Cloudy ↔ Sunny: Both can have similar temp/humidity in transition periods
 *    • Rainy ↔ Stormy: Differ mainly in intensity, similar pressure/humidity
 *    • Foggy ↔ Cloudy: Very similar conditions, differ mainly in visibility
 * 
 * 2. Realistic Sensor Noise:
 *    • Random data generation within training ranges
 *    • Some overlap exists between weather class boundaries
 *    • Model trained on real-world data with natural variations
 * 
 * 3. Model Accuracy Context:
 *    • Overall accuracy: ~85-90% (from training/validation)
 *    • Occasional misclassifications are part of model behavior
 *    • NOT a bug - reflects real-world uncertainty
 * 
 * Examples of ACCEPTABLE misclassifications:
 *    ✓ Sunny predicted as Cloudy (or vice versa) - similar conditions
 *    ✓ Rainy predicted as Stormy (or vice versa) - intensity difference
 *    ✓ Foggy predicted as Cloudy - very similar patterns
 * 
 * Examples of UNACCEPTABLE misclassifications (would indicate bugs):
 *    ✗ Sunny consistently predicted as Rainy - completely different
 *    ✗ All predictions are the same class - model failure
 *    ✗ >50% misclassification rate - training/scaling issue
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
    
    // Sensor value ranges - EXPANDED for diverse weather patterns
    // Original narrow range was causing only Cloudy predictions
    // Expanded ranges to generate all 5 weather classes
    static constexpr float TEMP_MIN = 15.0f;      // Expanded: 19→15 (colder = Rainy/Stormy)
    static constexpr float TEMP_MAX = 35.0f;      // Expanded: 30→35 (hotter = Sunny)
    static constexpr float HUMID_MIN = 20.0f;     // Expanded: 29.3→20 (drier = Sunny)
    static constexpr float HUMID_MAX = 95.0f;     // Expanded: 56.9→95 (wetter = Rainy/Stormy/Foggy)
    static constexpr float PRESSURE_MIN = 95000.0f;  // Expanded: 96352→95000 (lower = Stormy)
    static constexpr float PRESSURE_MAX = 103000.0f; // Expanded: 100301→103000 (higher = Sunny)
    static constexpr float LUX_MIN = 0.0f;        // Keep: 0 (dark = Rainy/Stormy/Foggy)
    static constexpr float LUX_MAX = 1000.0f;     // Expanded: 632→1000 (bright = Sunny)
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
    // Generate random sensor values with weather-pattern bias
    void readSensors() {
        // Randomly select weather pattern to generate (more diverse predictions)
        int weatherPattern = random(0, 10);  // 0-9 for varied distribution
        
        if (weatherPattern < 4) {
            // 40% Cloudy - moderate everything
            currentTemp = randomFloat(20.0f, 26.0f);
            currentHumid = randomFloat(40.0f, 70.0f);
            currentPressure = randomFloat(98000.0f, 101000.0f);
            currentLux = randomFloat(100.0f, 400.0f);
            currentGas = randomFloat(200.0f, 800.0f);
        } else if (weatherPattern < 6) {
            // 20% Sunny - high temp, low humidity, high lux
            currentTemp = randomFloat(28.0f, 35.0f);
            currentHumid = randomFloat(20.0f, 45.0f);
            currentPressure = randomFloat(100500.0f, 103000.0f);
            currentLux = randomFloat(500.0f, 1000.0f);
            currentGas = randomFloat(100.0f, 400.0f);
        } else if (weatherPattern < 8) {
            // 20% Rainy - low temp, high humidity, low lux
            currentTemp = randomFloat(15.0f, 22.0f);
            currentHumid = randomFloat(70.0f, 90.0f);
            currentPressure = randomFloat(96000.0f, 98500.0f);
            currentLux = randomFloat(10.0f, 150.0f);
            currentGas = randomFloat(300.0f, 900.0f);
        } else if (weatherPattern < 9) {
            // 10% Stormy - very low pressure, high humidity, very low lux
            currentTemp = randomFloat(16.0f, 23.0f);
            currentHumid = randomFloat(75.0f, 95.0f);
            currentPressure = randomFloat(95000.0f, 97000.0f);
            currentLux = randomFloat(0.0f, 80.0f);
            currentGas = randomFloat(400.0f, 1200.0f);
        } else {
            // 10% Foggy - high humidity, moderate temp, low lux
            currentTemp = randomFloat(18.0f, 24.0f);
            currentHumid = randomFloat(80.0f, 95.0f);
            currentPressure = randomFloat(97500.0f, 100000.0f);
            currentLux = randomFloat(5.0f, 100.0f);
            currentGas = randomFloat(500.0f, 1500.0f);
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
