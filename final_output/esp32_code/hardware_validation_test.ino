/*
 * ESP32-S3 Hardware Validation Test - Comprehensive Diagnostics
 * 
 * Purpose: Test WiFi + I2C sensor coexistence to diagnose interference issues
 * 
 * Test Sequence:
 * 1. Scan I2C bus and identify connected sensors
 * 2. Connect to WiFi
 * 3. Read sensor data for 5 seconds
 * 4. Verify WiFi still connected
 * 5. Disconnect WiFi
 * 6. Wait 3 seconds
 * 7. Repeat cycle
 * 
 * Hardware: ESP32-S3-DevKitC-1
 * I2C Pins: SDA=GPIO 8, SCL=GPIO 9
 * Analog: MQ2 Gas Sensor on GPIO 6
 * 
 * Expected Sensors:
 * - AHT10 (Temp/Humid) @ 0x38
 * - BME280 (Pressure) @ 0x76
 * - BH1750 (Light) @ 0x23
 * - MQ2 (Gas/Analog) @ GPIO 6
 * 
 * Connect sensors ONE BY ONE and watch for failures!
 */

#include <Wire.h>
#include <WiFi.h>

// ==================== CONFIGURATION ====================

// I2C Configuration
#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_FREQ 50000  // 50kHz for stability (reduced from 100kHz)

// WiFi Configuration
#define WIFI_SSID "COMFRI"
#define WIFI_PASSWORD "1234567890"
#define WIFI_TIMEOUT_MS 15000

// Analog Sensor
#define MQ2_PIN 6

// Test Parameters
#define SENSOR_READ_DURATION 5000   // Read sensors for 5 seconds
#define WIFI_OFF_DURATION 3000      // WiFi off for 3 seconds
#define CYCLE_DELAY 1000            // 1 second between readings

// ==================== GLOBAL VARIABLES ====================

// I2C Device Detection
struct I2CDevice {
    byte address;
    const char* name;
    bool detected;
};

I2CDevice knownDevices[] = {
    {0x23, "BH1750 (Light Sensor)", false},
    {0x38, "AHT10 (Temp/Humidity)", false},
    {0x76, "BME280 (Pressure)", false},
    {0x77, "BME280 (Pressure-Alt)", false}
};

const int numKnownDevices = sizeof(knownDevices) / sizeof(knownDevices[0]);

// Statistics
unsigned long testCycle = 0;
unsigned long totalWiFiConnections = 0;
unsigned long failedWiFiConnections = 0;
unsigned long wifiDropsDuringReading = 0;
unsigned long successfulCycles = 0;

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    delay(1000);
    
    printBanner();
    
    Serial.println("🔧 Hardware Validation Test Starting...");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println();
    
    Serial.println("⚠️  IMPORTANT: I2C pins NOT initialized yet");
    Serial.println("   Sensors are electrically invisible to ESP32");
    Serial.println("   I2C will activate ONLY after WiFi connects");
    Serial.println();
    
    // Initialize WiFi ONLY (I2C completely untouched)
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    delay(500);
    
    Serial.println("✅ Initialization complete");
    Serial.println();
    Serial.println("📋 Test will run continuously. Press RESET to stop.");
    Serial.println();
    
    delay(2000);
}

// ==================== MAIN LOOP ====================

void loop() {
    testCycle++;
    
    printCycleHeader();
    
    // STEP 1: Connect WiFi FIRST (before activating I2C)
    bool wifiConnected = connectWiFi();
    
    // STEP 2: Scan I2C Bus and Identify Sensors (after WiFi is stable)
    bool sensorsFound = false;
    if (wifiConnected) {
        sensorsFound = scanAndIdentifySensors();
    }
    
    if (wifiConnected && sensorsFound) {
        // STEP 3: Read Sensors for 5 seconds
        readSensorsWithMonitoring();
        
        // STEP 4: Verify WiFi Still Connected
        bool wifiStillConnected = checkWiFiStatus();
        
        if (wifiStillConnected) {
            successfulCycles++;
            Serial.println();
            Serial.println("✅ CYCLE PASSED: WiFi remained stable during sensor reading!");
        } else {
            Serial.println();
            Serial.println("❌ CYCLE FAILED: WiFi dropped during sensor reading!");
        }
        
        // STEP 5: Disconnect WiFi
        disconnectWiFi();
    } else if (wifiConnected && !sensorsFound) {
        // WiFi works but no sensors - still disconnect cleanly
        Serial.println("ℹ️  WiFi connected but no sensors detected");
        Serial.println("   Connect sensors to test interference!");
        disconnectWiFi();
    } else {
        Serial.println();
        Serial.println("❌ CYCLE FAILED: Could not connect to WiFi");
        Serial.println();
        Serial.println("   Possible causes:");
        Serial.println("      • Wrong WiFi credentials");
        Serial.println("      • Router out of range");
        Serial.println("      • Sensors already connected (remove them first!)");
        Serial.println();
    }
    
    // Print statistics
    printStatistics();
    
    // STEP 6: Wait 3 seconds before next cycle
    Serial.println();
    Serial.println("⏳ Waiting 3 seconds before next cycle...");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println();
    delay(WIFI_OFF_DURATION);
}

// ==================== STEP 1: I2C SCAN ====================

bool scanAndIdentifySensors() {
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║  STEP 2: I2C Bus Scan & Sensor Identification         ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    // Activate I2C bus (WiFi already connected, now testing coexistence)
    Serial.println("   🔌 Activating I2C Bus (WiFi already connected)...");
    Serial.println("   ⚠️  This is the critical moment - watch for WiFi drops!");
    Serial.println();
    
    Wire.setPins(I2C_SDA, I2C_SCL);
    Wire.begin();
    Wire.setClock(I2C_FREQ);
    delay(500);
    
    Serial.printf("   ✅ I2C Active: SDA=GPIO%d, SCL=GPIO%d, Speed=%dkHz\n", 
                  I2C_SDA, I2C_SCL, I2C_FREQ/1000);
    
    // Check if WiFi survived I2C activation
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("   ✅ WiFi still connected after I2C activation!");
    } else {
        Serial.println("   ❌ WiFi DROPPED when I2C was activated!");
        Serial.println("   This proves I2C interference is the root cause!");
    }
    Serial.println();
    
    // Reset detection flags
    for (int i = 0; i < numKnownDevices; i++) {
        knownDevices[i].detected = false;
    }
    
    // Scan I2C bus
    Serial.println("   📡 Scanning I2C addresses 0x01 to 0x7F...");
    Serial.println("   ─────────────────────────────────────────────────────");
    
    int foundDevices = 0;
    int unknownDevices = 0;
    
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            foundDevices++;
            
            // Check if it's a known device
            bool identified = false;
            for (int i = 0; i < numKnownDevices; i++) {
                if (knownDevices[i].address == addr) {
                    knownDevices[i].detected = true;
                    Serial.printf("   ✅ 0x%02X - %s\n", addr, knownDevices[i].name);
                    identified = true;
                    break;
                }
            }
            
            if (!identified) {
                Serial.printf("   ⚠️  0x%02X - Unknown Device\n", addr);
                unknownDevices++;
            }
        }
    }
    
    Serial.println("   ─────────────────────────────────────────────────────");
    Serial.printf("   📊 Summary: %d devices found", foundDevices);
    if (unknownDevices > 0) {
        Serial.printf(" (%d unknown)", unknownDevices);
    }
    Serial.println();
    
    // Check for MQ2 analog sensor
    Serial.println();
    Serial.println("   🌫️  Checking MQ-2 Gas Sensor (Analog GPIO 6)...");
    int gasValue = analogRead(MQ2_PIN);
    Serial.printf("   📊 MQ-2 Raw Value: %d ", gasValue);
    if (gasValue > 0 && gasValue < 4095) {
        Serial.println("✅ Sensor responding");
    } else if (gasValue == 0) {
        Serial.println("⚠️  Reading 0 (sensor may not be connected)");
    } else {
        Serial.println("⚠️  Reading max (check wiring)");
    }
    
    Serial.println();
    
    if (foundDevices == 0) {
        Serial.println("   ❌ NO I2C DEVICES FOUND!");
        Serial.println("   Possible causes:");
        Serial.println("      • No sensors connected");
        Serial.println("      • Wrong I2C pins (should be GPIO 8/9)");
        Serial.println("      • Missing pull-up resistors (2.2kΩ to 3.3V)");
        Serial.println("      • Bad wiring/connections");
    }
    
    Serial.println();
    return (foundDevices > 0);
}

// ==================== STEP 2: WiFi CONNECT ====================

bool connectWiFi() {
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║  STEP 1: WiFi Connection (I2C still isolated)         ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    Serial.printf("   📡 Connecting to '%s'...\n", WIFI_SSID);
    
    WiFi.disconnect();
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startTime = millis();
    int dots = 0;
    
    Serial.print("   ");
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
        dots++;
        if (dots % 40 == 0) {
            Serial.println();
            Serial.print("   ");
        }
        yield();  // Feed watchdog
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        totalWiFiConnections++;
        
        Serial.println();
        Serial.println("   ✅ WiFi CONNECTED!");
        Serial.println("   ─────────────────────────────────────────────────────");
        Serial.printf("   IP Address:    %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("   Gateway:       %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("   Signal (RSSI): %d dBm %s\n", WiFi.RSSI(), getSignalQuality(WiFi.RSSI()));
        Serial.printf("   Channel:       %d\n", WiFi.channel());
        Serial.printf("   Time taken:    %.2f seconds\n", (millis() - startTime) / 1000.0);
        Serial.println("   ─────────────────────────────────────────────────────");
        Serial.println();
        
        return true;
    } else {
        failedWiFiConnections++;
        
        Serial.println();
        Serial.println("   ❌ WiFi CONNECTION FAILED!");
        Serial.println("   ─────────────────────────────────────────────────────");
        Serial.printf("   Status Code: %d\n", WiFi.status());
        Serial.printf("   Status: %s\n", getWiFiStatusString(WiFi.status()));
        Serial.println("   ─────────────────────────────────────────────────────");
        Serial.println();
        Serial.println("   Possible causes:");
        Serial.println("      • Sensors connected causing power supply issues");
        Serial.println("      • No external 5V power for sensors");
        Serial.println("      • Missing decoupling capacitors");
        Serial.println("      • I2C cables too long (should be <15cm)");
        Serial.println("      • Missing I2C pull-up resistors");
        Serial.println();
        
        return false;
    }
}

// ==================== STEP 3: READ SENSORS ====================

void readSensorsWithMonitoring() {
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║  STEP 3: Reading Sensors (5 seconds with WiFi active) ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    unsigned long startTime = millis();
    int readingCount = 0;
    
    Serial.println("   📊 Sensor Readings:");
    Serial.println("   ─────────────────────────────────────────────────────");
    
    while (millis() - startTime < SENSOR_READ_DURATION) {
        readingCount++;
        
        Serial.printf("   [%d] ", readingCount);
        
        // Read each detected I2C sensor
        for (int i = 0; i < numKnownDevices; i++) {
            if (knownDevices[i].detected) {
                byte addr = knownDevices[i].address;
                
                // Try to read from sensor
                Wire.beginTransmission(addr);
                byte error = Wire.endTransmission();
                
                if (error == 0) {
                    Serial.printf("0x%02X:✅ ", addr);
                } else {
                    Serial.printf("0x%02X:❌ ", addr);
                }
            }
        }
        
        // Read MQ2 analog
        int gasValue = analogRead(MQ2_PIN);
        Serial.printf("| MQ2:%d ", gasValue);
        
        // Check WiFi status during reading
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("| WiFi:✅ (RSSI:%ddBm)", WiFi.RSSI());
        } else {
            Serial.print("| WiFi:❌ DROPPED!");
            wifiDropsDuringReading++;
        }
        
        Serial.println();
        
        delay(CYCLE_DELAY);
    }
    
    Serial.println("   ─────────────────────────────────────────────────────");
    Serial.printf("   ✅ Completed %d sensor readings in 5 seconds\n", readingCount);
    Serial.println();
}

// ==================== STEP 4: VERIFY WiFi ====================

bool checkWiFiStatus() {
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║  STEP 4: WiFi Status Verification                      ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("   ✅ WiFi STILL CONNECTED!");
        Serial.printf("   Signal: %d dBm %s\n", WiFi.RSSI(), getSignalQuality(WiFi.RSSI()));
        Serial.printf("   IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.println();
        return true;
    } else {
        Serial.println("   ❌ WiFi DISCONNECTED DURING SENSOR READING!");
        Serial.printf("   Status: %s\n", getWiFiStatusString(WiFi.status()));
        Serial.println();
        Serial.println("   🔍 This indicates:");
        Serial.println("      • I2C interference with WiFi radio");
        Serial.println("      • Power supply instability");
        Serial.println("      • Need for external sensor power");
        Serial.println("      • Need for decoupling capacitors");
        Serial.println();
        wifiDropsDuringReading++;
        return false;
    }
}

// ==================== STEP 5: DISCONNECT WiFi ====================

void disconnectWiFi() {
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║  STEP 5: WiFi Disconnect                               ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    WiFi.disconnect(true);
    delay(500);
    
    Serial.println("   📡 WiFi disconnected");
    Serial.println();
}

// ==================== UTILITIES ====================

void printCycleHeader() {
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.printf("║  TEST CYCLE #%-5lu                                     ║\n", testCycle);
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
}

void printStatistics() {
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║  Test Statistics                                       ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("   Total Cycles:          %lu\n", testCycle);
    Serial.printf("   Successful Cycles:     %lu\n", successfulCycles);
    Serial.printf("   WiFi Connections:      %lu\n", totalWiFiConnections);
    Serial.printf("   Failed Connections:    %lu\n", failedWiFiConnections);
    Serial.printf("   WiFi Drops (reading):  %lu\n", wifiDropsDuringReading);
    
    if (testCycle > 0) {
        float successRate = (successfulCycles * 100.0) / testCycle;
        Serial.printf("   Success Rate:          %.1f%%\n", successRate);
        
        if (successRate >= 95.0) {
            Serial.println("   Status: ✅ EXCELLENT - Hardware is stable!");
        } else if (successRate >= 80.0) {
            Serial.println("   Status: ⚠️  GOOD - Minor issues detected");
        } else if (successRate >= 50.0) {
            Serial.println("   Status: ⚠️  FAIR - Significant issues present");
        } else {
            Serial.println("   Status: ❌ POOR - Critical hardware problems!");
        }
    }
    
    Serial.println();
}

void printBanner() {
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║   ESP32-S3 Hardware Validation Test v1.0              ║");
    Serial.println("║   WiFi + I2C Coexistence Diagnostic Tool              ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("🎯 Purpose: Diagnose WiFi interference with I2C sensors");
    Serial.println();
    Serial.println("📋 Test Procedure:");
    Serial.println("   1. Connect sensors ONE BY ONE");
    Serial.println("   2. Watch for WiFi failures");
    Serial.println("   3. Identify which sensor causes problems");
    Serial.println();
    Serial.println("🔧 Expected Configuration:");
    Serial.println("   • I2C: GPIO 8 (SDA), GPIO 9 (SCL)");
    Serial.println("   • Analog: GPIO 6 (MQ2 Gas Sensor)");
    Serial.println("   • External 5V power for sensors (recommended)");
    Serial.println("   • Pull-up resistors: 2.2kΩ on SDA/SCL");
    Serial.println("   • Decoupling caps: 10µF on each sensor");
    Serial.println();
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println();
}

const char* getSignalQuality(int rssi) {
    if (rssi > -50) return "📶 Excellent";
    if (rssi > -60) return "📶 Good";
    if (rssi > -70) return "📶 Fair";
    return "📶 Weak";
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
