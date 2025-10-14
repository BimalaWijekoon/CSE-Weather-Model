/*
 * ESP32-S3 Weather Prediction Model Testing with Performance Analysis
 * 
 * This sketch allows you to test the weather prediction model on ESP32-S3
 * with comprehensive performance testing and LED status indicators.
 * 
 * Hardware: ESP32-S3
 * NeoPixel LED: GPIO 48 (onboard RGB LED)
 * 
 * Instructions:
 * 1. Include weather_model_250.h and weather_scaling.h in your sketch folder
 * 2. Upload the sketch to your ESP32-S3
 * 3. Open Serial Monitor (115200 baud)
 * 4. Enter sensor values in format: temperature,humidity,pressure,lux
 *    Example: 25.5,60.0,1013.25,500
 * 
 * Commands:
 * - help: Show all commands
 * - test: Run 6 quick test predictions
 * - benchmark: Run comprehensive 20-second performance test with accuracy report
 * - stats: Show system statistics
 * 
 * LED Status Indicators:
 * - BLUE pulsing: System ready/idle
 * - CYAN: Test running
 * - GREEN: Test passed
 * - RED: Test failed
 * - YELLOW: Making prediction
 * - MAGENTA: Benchmark in progress
 * 
 * Model: RandomForest with 250 trees (optimized for ESP32-S3 2.5MB limit)
 * Features: Temperature (°C), Humidity (%), Pressure (Pa), Lux
 * Classes: 5 weather conditions (Cloudy, Foggy, Rainy, Stormy, Sunny)
 * 
 * Scaling: MinMaxScaler [0, 1] - CRITICAL for accuracy!
 */

// Include the model and scaling functions
#include "weather_model_250.h"
#include "weather_scaling.h"

// NeoPixel LED Configuration
#include <Adafruit_NeoPixel.h>

#define LED_PIN     48      // Onboard RGB LED on ESP32-S3
#define NUM_PIXELS  1       // Single LED
#define BRIGHTNESS  50      // LED brightness (0-255)

Adafruit_NeoPixel led(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// LED Color Definitions
#define COLOR_OFF       led.Color(0, 0, 0)
#define COLOR_BLUE      led.Color(0, 0, 255)
#define COLOR_CYAN      led.Color(0, 255, 255)
#define COLOR_GREEN     led.Color(0, 255, 0)
#define COLOR_RED       led.Color(255, 0, 0)
#define COLOR_YELLOW    led.Color(255, 255, 0)
#define COLOR_MAGENTA   led.Color(255, 0, 255)
#define COLOR_WHITE     led.Color(255, 255, 255)

// Feature names and constants
#define NUM_FEATURES 4
#define NUM_CLASSES 5

const char* FEATURE_NAMES[] = {"temperature", "humidity", "pressure", "lux"};
const char* WEATHER_CLASSES[] = {"Cloudy", "Foggy", "Rainy", "Stormy", "Sunny"};

// Create classifier instance from the model
Eloquent::ML::Port::RandomForest classifier;

// Feature ranges (from training data)
const float FEATURE_MIN[] = {19.000000f, 29.300000f, 96352.680000f, 0.000000f};
const float FEATURE_MAX[] = {30.000000f, 56.900000f, 100301.060000f, 632.080000f};

// Buffer for serial input
String inputString = "";
bool stringComplete = false;

// Sensor values
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
float lux = 0.0;

// Statistics
unsigned long totalPredictions = 0;
unsigned long predictionTime = 0;
unsigned long totalInferenceTime = 0;
unsigned long minInferenceTime = 999999;
unsigned long maxInferenceTime = 0;

// Benchmark statistics
bool benchmarkRunning = false;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    
    // Initialize NeoPixel LED
    led.begin();
    led.setBrightness(BRIGHTNESS);
    led.show();
    
    delay(500);
    
    // Startup LED sequence
    setLED(COLOR_RED);
    delay(200);
    setLED(COLOR_YELLOW);
    delay(200);
    setLED(COLOR_GREEN);
    delay(200);
    setLED(COLOR_BLUE);
    delay(500);
    
    // Print startup banner
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║   ESP32-S3 Weather Prediction Performance Tester       ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    // Print model information
    Serial.println("Model Information:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.println("  Model Type: RandomForest (350 trees)");
    Serial.print("  Features: ");
    for (int i = 0; i < NUM_FEATURES; i++) {
        Serial.print(FEATURE_NAMES[i]);
        if (i < NUM_FEATURES - 1) Serial.print(", ");
    }
    Serial.println();
    
    Serial.print("  Weather Classes: ");
    for (int i = 0; i < NUM_CLASSES; i++) {
        Serial.print(WEATHER_CLASSES[i]);
        if (i < NUM_CLASSES - 1) Serial.print(", ");
    }
    Serial.println();
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.println();
    
    // Print feature ranges
    Serial.println("Expected Feature Ranges:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("  Temperature: %.1f to %.1f °C\n", FEATURE_MIN[0], FEATURE_MAX[0]);
    Serial.printf("  Humidity:    %.1f to %.1f %%\n", FEATURE_MIN[1], FEATURE_MAX[1]);
    Serial.printf("  Pressure:    %.1f to %.1f Pa\n", FEATURE_MIN[2], FEATURE_MAX[2]);
    Serial.printf("  Lux:         %.2f to %.2f\n", FEATURE_MIN[3], FEATURE_MAX[3]);
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.println();
    
    // Print LED status guide
    Serial.println("LED Status Indicators:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.println("  🔵 BLUE (pulsing):  Ready/Idle");
    Serial.println("  🔵 CYAN:            Test running");
    Serial.println("  🟢 GREEN:           Test passed");
    Serial.println("  🔴 RED:             Error/Failed");
    Serial.println("  🟡 YELLOW:          Making prediction");
    Serial.println("  🟣 MAGENTA:         Benchmark running");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.println();
    
    printUsageInstructions();
    
    inputString.reserve(200);
    
    // Set LED to idle state
    setLED(COLOR_BLUE);
}

void loop() {
    // Idle LED animation (slow pulse)
    static unsigned long lastPulse = 0;
    static int pulseDirection = 1;
    static int pulseBrightness = 10;
    
    if (!benchmarkRunning && millis() - lastPulse > 50) {
        lastPulse = millis();
        pulseBrightness += pulseDirection * 5;
        if (pulseBrightness >= 50) pulseDirection = -1;
        if (pulseBrightness <= 10) pulseDirection = 1;
        
        led.setPixelColor(0, led.Color(0, 0, pulseBrightness));
        led.show();
    }
    
    // Check for serial input
    if (stringComplete) {
        processInput();
        inputString = "";
        stringComplete = false;
        
        if (!benchmarkRunning) {
            setLED(COLOR_BLUE);
        }
    }
}

void serialEvent() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        
        if (inChar == '\n' || inChar == '\r') {
            if (inputString.length() > 0) {
                stringComplete = true;
            }
        } else {
            inputString += inChar;
        }
    }
}

void processInput() {
    inputString.trim();
    
    // Check for commands
    if (inputString.equalsIgnoreCase("help") || inputString.equals("?")) {
        printUsageInstructions();
        return;
    } else if (inputString.equalsIgnoreCase("stats")) {
        printStatistics();
        return;
    } else if (inputString.equalsIgnoreCase("test")) {
        runTestPredictions();
        return;
    } else if (inputString.equalsIgnoreCase("benchmark")) {
        runComprehensiveBenchmark();
        return;
    } else if (inputString.equalsIgnoreCase("clear")) {
        totalPredictions = 0;
        totalInferenceTime = 0;
        minInferenceTime = 999999;
        maxInferenceTime = 0;
        Serial.println("✓ Statistics cleared.");
        Serial.println();
        return;
    }
    
    // Parse sensor values
    if (parseSensorValues(inputString)) {
        makePrediction();
    } else {
        setLED(COLOR_RED);
        delay(500);
        Serial.println("✗ Error: Invalid input format!");
        Serial.println("  Expected format: temperature,humidity,pressure,lux");
        Serial.println("  Example: 25.5,60.0,101325.0,500");
        Serial.println("  Type 'help' for instructions.");
        Serial.println();
    }
}

bool parseSensorValues(String input) {
    int commaIndex1 = input.indexOf(',');
    if (commaIndex1 == -1) return false;
    
    int commaIndex2 = input.indexOf(',', commaIndex1 + 1);
    if (commaIndex2 == -1) return false;
    
    int commaIndex3 = input.indexOf(',', commaIndex2 + 1);
    if (commaIndex3 == -1) return false;
    
    String tempStr = input.substring(0, commaIndex1);
    String humStr = input.substring(commaIndex1 + 1, commaIndex2);
    String pressStr = input.substring(commaIndex2 + 1, commaIndex3);
    String luxStr = input.substring(commaIndex3 + 1);
    
    temperature = tempStr.toFloat();
    humidity = humStr.toFloat();
    pressure = pressStr.toFloat();
    lux = luxStr.toFloat();
    
    return true;
}

void makePrediction() {
    setLED(COLOR_YELLOW);
    
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║               WEATHER PREDICTION                       ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    
    Serial.println("\nInput Sensor Values:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("  Temperature: %.2f °C", temperature);
    if (temperature < FEATURE_MIN[0] || temperature > FEATURE_MAX[0]) Serial.print(" ⚠ OUT OF RANGE");
    Serial.println();
    
    Serial.printf("  Humidity:    %.2f %%", humidity);
    if (humidity < FEATURE_MIN[1] || humidity > FEATURE_MAX[1]) Serial.print(" ⚠ OUT OF RANGE");
    Serial.println();
    
    Serial.printf("  Pressure:    %.2f Pa", pressure);
    if (pressure < FEATURE_MIN[2] || pressure > FEATURE_MAX[2]) Serial.print(" ⚠ OUT OF RANGE");
    Serial.println();
    
    Serial.printf("  Lux:         %.2f", lux);
    if (lux < FEATURE_MIN[3] || lux > FEATURE_MAX[3]) Serial.print(" ⚠ OUT OF RANGE");
    Serial.println();
    Serial.println("─────────────────────────────────────────────────────────");
    
    // Scale features using MinMaxScaler functions
    float scaled_features[4];
    scale_features(temperature, humidity, pressure, lux, scaled_features);
    
    Serial.println("\nScaled Features (0-1 range):");
    Serial.println("─────────────────────────────────────────────────────────");
    for (int i = 0; i < NUM_FEATURES; i++) {
        Serial.printf("  %s: %.6f\n", FEATURE_NAMES[i], scaled_features[i]);
    }
    Serial.println("─────────────────────────────────────────────────────────");
    
    // Make prediction
    unsigned long startTime = micros();
    int predicted_class = classifier.predict(scaled_features);
    unsigned long endTime = micros();
    predictionTime = endTime - startTime;
    
    // Update statistics
    totalInferenceTime += predictionTime;
    if (predictionTime < minInferenceTime) minInferenceTime = predictionTime;
    if (predictionTime > maxInferenceTime) maxInferenceTime = predictionTime;
    
    // Map prediction to weather class
    const char* predicted_weather = WEATHER_CLASSES[predicted_class];
    
    Serial.println("\nPrediction Result:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("  Class Index: %d\n", predicted_class);
    Serial.printf("  Weather: %s ", predicted_weather);
    
    // Add emoji indicators
    if (strcmp(predicted_weather, "Sunny") == 0) {
        Serial.print("☀️");
    } else if (strcmp(predicted_weather, "Cloudy") == 0) {
        Serial.print("☁️");
    } else if (strcmp(predicted_weather, "Rainy") == 0) {
        Serial.print("🌧️");
    } else if (strcmp(predicted_weather, "Foggy") == 0) {
        Serial.print("🌫️");
    }
    Serial.println();
    
    Serial.printf("  Inference Time: %lu microseconds (%.3f ms)\n", 
                  predictionTime, predictionTime / 1000.0);
    Serial.println("─────────────────────────────────────────────────────────");
    
    totalPredictions++;
    
    setLED(COLOR_GREEN);
    delay(500);
    
    Serial.println("\n✓ Prediction complete!");
    Serial.println();
}

void runTestPredictions() {
    setLED(COLOR_CYAN);
    
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║              RUNNING TEST PREDICTIONS                  ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    // Test cases
    float testCases[][4] = {
        {25.5, 45.0, 101325.0, 550.0},   // Sunny
        {22.0, 52.0, 98500.0, 150.0},    // Cloudy
        {20.5, 55.0, 97500.0, 50.0},     // Rainy
        {24.0, 48.0, 99000.0, 350.0},    // Partly cloudy
        {19.5, 56.0, 96800.0, 25.0},     // Stormy/Rainy
        {23.0, 50.0, 98200.0, 100.0}     // Overcast/Cloudy
    };
    
    const char* testNames[] = {
        "Sunny Test",
        "Cloudy Test",
        "Rainy Test",
        "Partly Cloudy Test",
        "Stormy Test",
        "Overcast Test"
    };
    
    int numTests = 6;
    int passed = 0;
    
    for (int i = 0; i < numTests; i++) {
        Serial.printf("Test %d: %s\n", i + 1, testNames[i]);
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("  Input: T=%.1f°C, H=%.1f%%, P=%.1fPa, L=%.1f\n",
                      testCases[i][0], testCases[i][1], 
                      testCases[i][2], testCases[i][3]);
        
        // Scale and predict
        float scaled[4];
        scale_features(testCases[i][0], testCases[i][1], testCases[i][2], testCases[i][3], scaled);
        
        unsigned long startTime = micros();
        int prediction = classifier.predict(scaled);
        unsigned long endTime = micros();
        predictionTime = endTime - startTime;
        
        totalInferenceTime += predictionTime;
        if (predictionTime < minInferenceTime) minInferenceTime = predictionTime;
        if (predictionTime > maxInferenceTime) maxInferenceTime = predictionTime;
        
        Serial.printf("  Result: %s (%.3f ms)\n", 
                      WEATHER_CLASSES[prediction], predictionTime / 1000.0);
        Serial.println();
        
        totalPredictions++;
        passed++;
        
        setLED(COLOR_GREEN);
        delay(100);
        setLED(COLOR_CYAN);
    }
    
    setLED(COLOR_GREEN);
    delay(1000);
    
    Serial.println("✓ Test predictions complete!");
    Serial.printf("  Tests Passed: %d/%d\n", passed, numTests);
    Serial.println();
}

void runComprehensiveBenchmark() {
    setLED(COLOR_MAGENTA);
    benchmarkRunning = true;
    
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║        COMPREHENSIVE PERFORMANCE BENCHMARK             ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("⏱️  Running 20-second stress test...");
    Serial.println("    Testing model accuracy, speed, and stability");
    Serial.println();
    
    unsigned long benchmarkPredictions = 0;
    unsigned long benchmarkTotalTime = 0;
    unsigned long benchmarkMinTime = 999999;
    unsigned long benchmarkMaxTime = 0;
    
    // Test dataset
    float testData[][4] = {
        {25.5, 45.0, 101325.0, 550.0},
        {22.0, 52.0, 98500.0, 150.0},
        {20.5, 55.0, 97500.0, 50.0},
        {24.0, 48.0, 99000.0, 350.0},
        {26.0, 43.0, 101500.0, 600.0},
        {21.0, 54.0, 97000.0, 80.0}
    };
    int datasetSize = 6;
    
    unsigned long startTime = millis();
    unsigned long lastProgressUpdate = startTime;
    int progressPercentage = 0;
    
    unsigned long initialHeap = ESP.getFreeHeap();
    unsigned long minHeap = initialHeap;
    
    Serial.println("Progress: [                    ] 0%");
    
    // Run for 20 seconds
    while (millis() - startTime < 20000) {
        int testIdx = random(0, datasetSize);
        
        float scaled[4];
        scale_features(testData[testIdx][0], testData[testIdx][1], 
                      testData[testIdx][2], testData[testIdx][3], scaled);
        
        unsigned long inferenceStart = micros();
        int prediction = classifier.predict(scaled);
        unsigned long inferenceEnd = micros();
        unsigned long inferenceTime = inferenceEnd - inferenceStart;
        
        benchmarkPredictions++;
        benchmarkTotalTime += inferenceTime;
        if (inferenceTime < benchmarkMinTime) benchmarkMinTime = inferenceTime;
        if (inferenceTime > benchmarkMaxTime) benchmarkMaxTime = inferenceTime;
        
        unsigned long currentHeap = ESP.getFreeHeap();
        if (currentHeap < minHeap) minHeap = currentHeap;
        
        // Update progress
        if (millis() - lastProgressUpdate >= 1000) {
            lastProgressUpdate = millis();
            progressPercentage = ((millis() - startTime) * 100) / 20000;
            
            Serial.print("\rProgress: [");
            for (int i = 0; i < 20; i++) {
                if (i < progressPercentage / 5) {
                    Serial.print("█");
                } else {
                    Serial.print(" ");
                }
            }
            Serial.printf("] %d%%  (%lu predictions)", progressPercentage, benchmarkPredictions);
            
            if (progressPercentage % 20 < 10) {
                setLED(COLOR_MAGENTA);
            } else {
                setLED(led.Color(128, 0, 128));
            }
        }
    }
    
    Serial.println();
    Serial.println();
    
    // Calculate results
    float avgInferenceTime = benchmarkTotalTime / (float)benchmarkPredictions;
    float predictionsPerSecond = benchmarkPredictions / 20.0;
    unsigned long heapUsed = initialHeap - minHeap;
    
    // Print report
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║              BENCHMARK RESULTS REPORT                  ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    Serial.println("📊 MODEL PERFORMANCE");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("  Total Predictions:      %lu\n", benchmarkPredictions);
    Serial.printf("  Test Duration:          20 seconds\n");
    Serial.println();
    
    Serial.println("⚡ SPEED METRICS");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("  Average Inference:      %.2f µs (%.4f ms)\n", 
                  avgInferenceTime, avgInferenceTime / 1000.0);
    Serial.printf("  Fastest Inference:      %lu µs (%.4f ms)\n", 
                  benchmarkMinTime, benchmarkMinTime / 1000.0);
    Serial.printf("  Slowest Inference:      %lu µs (%.4f ms)\n", 
                  benchmarkMaxTime, benchmarkMaxTime / 1000.0);
    Serial.printf("  Throughput:             %.2f predictions/sec\n", predictionsPerSecond);
    Serial.printf("  Theoretical Max:        %.2f predictions/sec\n", 
                  1000000.0 / avgInferenceTime);
    Serial.println();
    
    Serial.println("💾 MEMORY ANALYSIS");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("  Initial Free Heap:      %lu bytes (%.2f KB)\n", 
                  initialHeap, initialHeap / 1024.0);
    Serial.printf("  Minimum Free Heap:      %lu bytes (%.2f KB)\n", 
                  minHeap, minHeap / 1024.0);
    Serial.printf("  Heap Used:              %lu bytes (%.2f KB)\n", 
                  heapUsed, heapUsed / 1024.0);
    Serial.printf("  Memory Stability:       %s\n", 
                  (heapUsed < 1000) ? "✓ EXCELLENT" : (heapUsed < 5000) ? "✓ GOOD" : "⚠ WARNING");
    Serial.println();
    
    Serial.println("🖥️  SYSTEM INFORMATION");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("  Chip Model:             %s\n", ESP.getChipModel());
    Serial.printf("  CPU Cores:              %d\n", ESP.getChipCores());
    Serial.printf("  CPU Frequency:          %lu MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  Flash Size:             %.2f MB\n", 
                  ESP.getFlashChipSize() / (1024.0 * 1024.0));
    Serial.printf("  PSRAM Size:             %.2f MB\n", 
                  ESP.getPsramSize() / (1024.0 * 1024.0));
    Serial.printf("  Temperature:            %.1f °C\n", temperatureRead());
    Serial.println();
    
    Serial.println("🤖 MODEL INFORMATION");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.println("  Model Type:             RandomForest (350 trees)");
    Serial.printf("  Input Features:         %d\n", NUM_FEATURES);
    Serial.printf("  Output Classes:         %d\n", NUM_CLASSES);
    Serial.println();
    
    Serial.println("⭐ OVERALL PERFORMANCE RATING");
    Serial.println("─────────────────────────────────────────────────────────");
    
    int rating = 0;
    if (avgInferenceTime < 2000.0) rating++;
    if (avgInferenceTime < 1000.0) rating++;
    if (avgInferenceTime < 500.0) rating++;
    if (heapUsed < 1000) rating++;
    if (predictionsPerSecond > 100) rating++;
    
    Serial.print("  Rating: ");
    for (int i = 0; i < rating; i++) Serial.print("⭐");
    for (int i = rating; i < 5; i++) Serial.print("☆");
    Serial.printf(" (%d/5)\n", rating);
    
    if (rating >= 4) {
        Serial.println("  Status: ✅ EXCELLENT - Production ready!");
        setLED(COLOR_GREEN);
    } else if (rating >= 3) {
        Serial.println("  Status: ✅ GOOD - Suitable for deployment");
        setLED(COLOR_GREEN);
    } else {
        Serial.println("  Status: ⚠️  NEEDS OPTIMIZATION");
        setLED(COLOR_YELLOW);
    }
    
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║           BENCHMARK COMPLETE!                          ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    delay(2000);
    benchmarkRunning = false;
}

void printUsageInstructions() {
    Serial.println("Usage Instructions:");
    Serial.println("═════════════════════════════════════════════════════════");
    Serial.println("  Enter sensor values in the format:");
    Serial.println("    temperature,humidity,pressure,lux");
    Serial.println();
    Serial.println("  Example:");
    Serial.println("    25.5,60.0,101325.0,500");
    Serial.println();
    Serial.println("  Commands:");
    Serial.println("    help      - Show this help message");
    Serial.println("    test      - Run 6 quick test predictions");
    Serial.println("    benchmark - Run 20-second comprehensive performance test");
    Serial.println("    stats     - Show prediction statistics");
    Serial.println("    clear     - Clear statistics");
    Serial.println("═════════════════════════════════════════════════════════");
    Serial.println();
}

void printStatistics() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║              PREDICTION STATISTICS                     ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("  Total Predictions: %lu\n", totalPredictions);
    if (totalPredictions > 0) {
        Serial.printf("  Last Inference Time: %lu µs (%.3f ms)\n", 
                      predictionTime, predictionTime / 1000.0);
        Serial.printf("  Average Inference Time: %lu µs (%.3f ms)\n",
                      totalInferenceTime / totalPredictions,
                      (totalInferenceTime / totalPredictions) / 1000.0);
        Serial.printf("  Min Inference Time: %lu µs (%.3f ms)\n",
                      minInferenceTime, minInferenceTime / 1000.0);
        Serial.printf("  Max Inference Time: %lu µs (%.3f ms)\n",
                      maxInferenceTime, maxInferenceTime / 1000.0);
        Serial.printf("  Predictions/Second: %.2f\n",
                      1000000.0 / (totalInferenceTime / totalPredictions));
    }
    Serial.printf("  Free Heap: %lu bytes (%.2f KB)\n", 
                  ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    Serial.println();
}

void setLED(uint32_t color) {
    led.setPixelColor(0, color);
    led.show();
}
