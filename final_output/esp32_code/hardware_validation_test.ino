/*
 * ESP32-S3 Hardware Validation Test - Simplified
 * 
 * Purpose: Test WiFi + I2C sensor coexistence
 * 
 * Test Sequence:
 * 1. Connect to WiFi
 * 2. Scan I2C bus and list all devices found
 * 3. Read sensors continuously
 * 
 * Hardware: ESP32-S3-DevKitC-1
 * I2C Pins: SDA=GPIO 8, SCL=GPIO 9
 * Analog: MQ2 Gas Sensor on GPIO 6
 */

#include <Wire.h>
#include <WiFi.h>

// ==================== CONFIGURATION ====================

// I2C Configuration
#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_FREQ 100000  // 100kHz standard speed
#define I2C_TIMEOUT_MS 50  // Short timeout for faster scanning

// WiFi Configuration
#define WIFI_SSID "COMFRI"
#define WIFI_PASSWORD "1234567890"
#define WIFI_TIMEOUT_MS 15000

// Analog Sensor
#define MQ2_PIN 6

// Test Parameters
#define SENSOR_READ_INTERVAL 2000   // Read sensors every 2 seconds

// ==================== GLOBAL VARIABLES ====================

// Store detected I2C addresses
byte i2cDevices[127];
int numI2CDevices = 0;

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    delay(1000);
    
    printBanner();
    
    // STEP 1: Connect to WiFi first
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("STEP 1: Connecting to WiFi");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println();
    
    connectWiFi();
    
    // STEP 2: Initialize I2C and scan for devices
    Serial.println();
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("STEP 2: Initializing I2C and Scanning for Devices");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println();
    
    Wire.setPins(I2C_SDA, I2C_SCL);
    Wire.begin();
    Wire.setClock(I2C_FREQ);
    Wire.setTimeOut(I2C_TIMEOUT_MS);  // Set short timeout for scanning
    delay(100);
    
    Serial.printf("✅ I2C initialized: SDA=GPIO%d, SCL=GPIO%d, Speed=%dkHz\n", 
                  I2C_SDA, I2C_SCL, I2C_FREQ/1000);
    Serial.println();
    
    scanI2CDevices();
    
    Serial.println();
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("Setup complete. Starting sensor readings...");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println();
}

// ==================== MAIN LOOP ====================

void loop() {
    // Check WiFi status
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("✅ WiFi: Connected | RSSI: %d dBm | IP: %s\n", 
                      WiFi.RSSI(), WiFi.localIP().toString().c_str());
    } else {
        Serial.println("❌ WiFi: Disconnected - Reconnecting...");
        connectWiFi();
    }
    
    // Read all I2C sensors
    Serial.println("\n📊 Sensor Readings:");
    Serial.println("─────────────────────────────────────────");
    
    for (int i = 0; i < numI2CDevices; i++) {
        byte addr = i2cDevices[i];
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        
        Serial.printf("  I2C 0x%02X: %s\n", addr, (error == 0) ? "✅ OK" : "❌ Error");
    }
    
    // Read analog sensor
    int gasValue = analogRead(MQ2_PIN);
    Serial.printf("  MQ2 (GPIO%d): %d\n", MQ2_PIN, gasValue);
    
    Serial.println("─────────────────────────────────────────");
    
    delay(SENSOR_READ_INTERVAL);
}

// ==================== I2C SCAN ====================

void scanI2CDevices() {
    Serial.println("📡 Scanning I2C bus...");
    Serial.println("   (Scanning common addresses: 0x08-0x77)");
    Serial.println();
    
    numI2CDevices = 0;
    int scanned = 0;
    
    // Scan only common I2C addresses (skip reserved addresses)
    // 0x00-0x07 are reserved, 0x78-0x7F are reserved
    for (byte addr = 0x08; addr < 0x78; addr++) {
        scanned++;
        
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            i2cDevices[numI2CDevices] = addr;
            numI2CDevices++;
            Serial.printf("  ✅ Device found at address 0x%02X (%d)\n", addr, addr);
        }
        
        // Small delay to prevent flooding
        delay(5);
    }
    
    Serial.println();
    Serial.printf("   Scanned %d addresses\n", scanned);
    
    if (numI2CDevices == 0) {
        Serial.println("❌ No I2C devices found!");
        Serial.println();
        Serial.println("   Troubleshooting:");
        Serial.println("   • Check wiring: SDA=GPIO8, SCL=GPIO9");
        Serial.println("   • Verify 3.3V power to sensors");
        Serial.println("   • Add pull-up resistors (2.2kΩ-4.7kΩ) on SDA & SCL");
        Serial.println("   • Check sensor power indicator LEDs");
    } else {
        Serial.printf("✅ Found %d I2C device(s)\n", numI2CDevices);
        Serial.println();
        Serial.println("   Common I2C Addresses:");
        Serial.println("   • 0x23 = BH1750 (Light)");
        Serial.println("   • 0x38 = AHT10 (Temp/Humidity)");
        Serial.println("   • 0x76 = BME280 (Pressure)");
        Serial.println("   • 0x77 = BME280 (Alt address)");
    }
}

// ==================== WiFi CONNECT ====================

void connectWiFi() {
    Serial.printf("📡 Connecting to '%s'", WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi Connected!");
        Serial.printf("   IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("   Signal: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("❌ WiFi Connection Failed!");
        Serial.printf("   Status: %s\n", getWiFiStatusString(WiFi.status()));
    }
}

// ==================== UTILITIES ====================

void printBanner() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║   ESP32-S3 Hardware Validation Test - Simplified      ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println(" Configuration:");
    Serial.println("   • I2C: GPIO 8 (SDA), GPIO 9 (SCL)");
    Serial.println("   • Analog: GPIO 6 (MQ2)");
    Serial.println();
}

const char* getWiFiStatusString(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS: return "Idle";
        case WL_NO_SSID_AVAIL: return "SSID not found";
        case WL_SCAN_COMPLETED: return "Scan completed";
        case WL_CONNECTED: return "Connected";
        case WL_CONNECT_FAILED: return "Connection failed";
        case WL_CONNECTION_LOST: return "Connection lost";
        case WL_DISCONNECTED: return "Disconnected";
        default: return "Unknown";
    }
}
