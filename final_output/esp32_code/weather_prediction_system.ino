/*
 * ESP32-S3 Weather Prediction System - MODULAR ARCHITECTURE v3.0
 * 
 * Architecture:
 * - Startup: WiFi + Cloud connections ONLY (NO sensor init)
 * - Commands:
 *   • "sensortest" - Test real hardware sensors (15 readings, 15 seconds)
 *   • "startsim"   - Start continuous simulation mode
 * 
 * Hardware: ESP32-S3-DevKitC-1
 * I2C Pins: SDA=GPIO 8, SCL=GPIO 9
 * Analog: MQ2 Gas Sensor on GPIO 6
 * 
 * Sensors:
 * - AHT10 (Temp/Humid) @ 0x38
 * - BME280 (Pressure) @ 0x76
 * - BH1750 (Light) @ 0x23
 * - MQ2 (Gas/Analog) @ GPIO 6
 * 
 * Features:
 * - Software I2C isolation during WiFi init
 * - 15-second data averaging for stability
 * - ThingSpeak cloud integration (15s rate limit compliant)
 * - ML weather prediction (RandomForest 250 trees)
 */

#include <Wire.h>

// Include ML model and scaling
#include "weather_model_250.h"
#include "weather_scaling.h"

// Include modular components
#include "wifi_manager.h"
#include "cloud_manager.h"
#include "firebase_manager.h"
#include "sensor_aht10.h"
#include "sensor_bme280.h"
#include "sensor_bh1750.h"
#include "sensor_mq2.h"
#include "sensor_test.h"
#include "sensor_simulate.h"

// ==================== CONFIGURATION ====================

// I2C Pins
#define I2C_SDA 8
#define I2C_SCL 9

// ThingSpeak
#define THINGSPEAK_API_KEY "J3GFLQKI0TVR6JC2"
#define THINGSPEAK_CHANNEL_ID "3108323"

// Firebase
#define FIREBASE_ENABLED false

// ==================== GLOBAL OBJECTS ====================

// Managers
WiFiManager wifiManager;
CloudManager cloudManager(THINGSPEAK_API_KEY, THINGSPEAK_CHANNEL_ID);
FirebaseManager firebaseManager;  // Firebase enabled/disabled via FIREBASE_ENABLED define in firebase_manager.h

// Sensors (created but NOT initialized at startup)
AHT10Sensor aht10;
BME280Sensor bme280;
BH1750Sensor bh1750;
MQ2Sensor mq2;

// ML Classifier
Eloquent::ML::Port::RandomForest classifier;

// Test and Simulation modules
SensorTest* sensorTest;
SensorSimulator simulator;

// System state
String inputString = "";
bool stringComplete = false;

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    delay(1000);
    
    // ========== SIMULATION MODE ONLY - REAL SENSORS DISABLED ==========
    // COMMENTED OUT: Real sensor I2C initialization to avoid WiFi interference
    // Uncomment these lines when you have proper hardware setup:
    //   - External 5V power supply for sensors
    //   - 2.2kΩ pull-up resistors on SDA/SCL
    //   - 10µF decoupling capacitors on each sensor
    //   - 100µF main power capacitor
    
    // pinMode(I2C_SDA, INPUT);  // GPIO 8 = high-impedance
    // pinMode(I2C_SCL, INPUT);  // GPIO 9 = high-impedance
    
    Serial.println("⚠️  SIMULATION MODE: Real sensors are DISABLED");
    Serial.println("   I2C pins not initialized - prevents WiFi interference");
    Serial.println("   Use 'startsim' command for data generation");
    Serial.println();
    
    printBanner();
    
    // Step 1: WiFi Connection
    wifiManager.begin();
    wifiManager.connect();
    
    // Step 2: ThingSpeak Connection Test
    cloudManager.testConnection();
    
    // CRITICAL: Allow HTTP client to fully release before any future uploads
    // ESP32 HTTPClient requires time to properly close connections
    Serial.println("   ⏳ Waiting 2 seconds for HTTP connection to release...");
    delay(2000);
    Serial.println("   ✅ HTTP client ready for future requests");
    Serial.println();
    
    // Step 3: Firebase Init
    firebaseManager.initialize();
    
    // COMMENTED OUT: Real sensor test module
    // Uncomment when hardware fixes are complete
    // sensorTest = new SensorTest(I2C_SDA, I2C_SCL, 
    //                              &aht10, &bme280, &bh1750, &mq2,
    //                              &cloudManager, &firebaseManager, &classifier);
    
    simulator.begin();
    simulator.setWiFiStatus(wifiManager.isConnected());
    simulator.setFirebaseManager(&firebaseManager);
    
    // Ready!
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║              ✅ SYSTEM READY - SIMULATION MODE         ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("💡 Available Commands:");
    Serial.println("   • startsim   - Start continuous simulation (RECOMMENDED)");
    Serial.println("   • help       - Show command help");
    Serial.println();
    Serial.println("⚠️  Note: 'sensortest' command disabled (hardware not configured)");
    Serial.println();
    
    inputString.reserve(200);
}

// ==================== MAIN LOOP ====================

void loop() {
    // CRITICAL: Monitor WiFi connection status continuously
    // This detects disconnections and can trigger auto-reconnect
    wifiManager.update();
    
    // Update WiFi status for simulator
    simulator.setWiFiStatus(wifiManager.isConnected());
    
    // Update simulator (if running)
    simulator.update();
    
    // Check for serial input
    if (stringComplete) {
        // Stop simulation if any key pressed while running
        if (simulator.running()) {
            simulator.stop();
        } else {
            processCommand();
        }
        inputString = "";
        stringComplete = false;
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

// ==================== COMMAND PROCESSING ====================

void processCommand() {
    inputString.trim();
    inputString.toLowerCase();
    
    if (inputString == "sensortest") {
        // DISABLED: Real sensor testing (hardware not configured)
        Serial.println();
        Serial.println("❌ 'sensortest' command is currently DISABLED");
        Serial.println();
        Serial.println("Reason: Real sensors cause WiFi interference without proper hardware fixes");
        Serial.println();
        Serial.println("Required hardware to enable:");
        Serial.println("   ✓ External 5V power supply for sensors");
        Serial.println("   ✓ 2× 2.2kΩ pull-up resistors on SDA/SCL");
        Serial.println("   ✓ 4× 10µF decoupling capacitors (one per sensor)");
        Serial.println("   ✓ 1× 100µF main power capacitor");
        Serial.println("   ✓ Short I2C cables (<15cm, twisted)");
        Serial.println();
        Serial.println("💡 For now, use 'startsim' for weather prediction testing");
        Serial.println();
        
        // Uncomment this line when hardware is ready:
        // sensorTest->run();
    } else if (inputString == "startsim") {
        simulator.start();
    } else if (inputString == "help") {
        printHelp();
    } else {
        Serial.println("❌ Unknown command");
        Serial.println("💡 Type 'help' for available commands");
        Serial.println();
    }
}

// ==================== UI ====================

void printBanner() {
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║      ESP32-S3 Weather Prediction System v3.0          ║");
    Serial.println("║         MODULAR ARCHITECTURE - CLEAN CODE              ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
}

void printHelp() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║                  AVAILABLE COMMANDS                    ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("📋 Command List:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.println("   startsim   - Start continuous simulation (RECOMMENDED)");
    Serial.println("                • Random sensor data generation");
    Serial.println("                • 1 reading/second");
    Serial.println("                • Prediction every 15 seconds");
    Serial.println("                • Auto cloud upload with retry logic");
    Serial.println("                • Press ANY KEY to stop");
    Serial.println();
    Serial.println("   sensortest - Test real hardware sensors (DISABLED)");
    Serial.println("                ⚠️  Requires external 5V power & proper wiring");
    Serial.println("                ⚠️  See WIRING_DIAGRAM_FIXED.txt for details");
    Serial.println();
    Serial.println("   help       - Show this help message");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.println();
    Serial.println("ℹ️  NOTE: Model may occasionally confuse similar conditions");
    Serial.println("   (Sunny↔Cloudy, Rainy↔Stormy) - this is normal behavior");
    Serial.println();
}
