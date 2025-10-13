/*
* Firebase Manager - Cloud Backup Integration
*
* Features:
* - Automatic data backup to Firebase Realtime Database
* - Unique device ID (MAC address based)
* - Separate fields for each sensor reading
* - Prediction history storage with timestamps
* - Device metadata tracking
* - Configurable backup intervals
* - Failure handling and statistics
*
* Database Structure:
* /devices/{device_id}/
*   ├─ info/ (Device metadata)
*   │  ├─ device_id
*   │  ├─ firmware_version
*   │  ├─ model_type
*   │  ├─ chip_model
*   │  ├─ mac_address
*   │  └─ last_boot
*   ├─ status/ (Current status)
*   │  ├─ online
*   │  └─ last_seen
*   └─ readings/{timestamp}/ (Sensor readings)
*      ├─ temperature
*      ├─ humidity
*      ├─ pressure
*      ├─ lux
*      ├─ gas_ppm (optional)
*      ├─ gas_quality (optional)
*      ├─ prediction
*      ├─ inference_time
*      └─ timestamp
*
* Library Required:
* - "Firebase Arduino Client Library for ESP8266 and ESP32" by Mobizt
* - Install via: Tools → Manage Libraries → Search "Firebase ESP32"
*/

#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>

// ==================== CONFIGURATION ====================
// Firebase project credentials
#define FIREBASE_HOST "https://weather-prediction-syste-9d94d-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_API_KEY "AIzaSyDO5pK4EIi6N4n9PaZZ4GvS7wh52aPsiyU"

// Firebase Authentication (REQUIRED - not optional!)
#define FIREBASE_USER_EMAIL "esp32@test.com"
#define FIREBASE_USER_PASSWORD "esp32test123"

// Backup settings
#define FIREBASE_ENABLED true      // ✅ ENABLED - Library installed
#define BACKUP_INTERVAL 15000      // Backup every 15 seconds for testing (was 300000 = 5 min)
#define MAX_FAILED_UPLOADS 10      // Stop trying after 10 failures

// Firebase library includes
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// ==================== FIREBASE MANAGER ====================
class FirebaseManager {
private:
  // Firebase objects
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;
  
  // Configuration
  String deviceID;
  bool enabled;
  bool initialized;
  bool connected;
  unsigned long backupInterval;
  unsigned long maxConsecutiveFailures;
  
  // Timing
  unsigned long lastBackupTime;
  unsigned long readingCount;
  
  // Statistics
  unsigned long totalBackups;
  unsigned long successfulBackups;
  unsigned long failedBackups;
  unsigned long consecutiveFailures;

public:
  FirebaseManager() {
    deviceID = "";
    enabled = FIREBASE_ENABLED;
    initialized = false;
    connected = false;
    backupInterval = BACKUP_INTERVAL;
    maxConsecutiveFailures = MAX_FAILED_UPLOADS;
    lastBackupTime = 0;
    readingCount = 0;
    totalBackups = 0;
    successfulBackups = 0;
    failedBackups = 0;
    consecutiveFailures = 0;
  }

  // ==================== INITIALIZATION ====================
  void initialize() {
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("STEP 3: Firebase Backup");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // Generate unique device ID from MAC address
    deviceID = generateDeviceID();
    Serial.printf("   Device ID: %s\n", deviceID.c_str());
    Serial.printf("   Database: %s\n", FIREBASE_HOST);
    Serial.printf("   Interval: %lu seconds\n", backupInterval / 1000);
    
    if (!enabled) {
      // Run in simulated mode (shows messages without actual Firebase connection)
      initialized = true;  // Allow simulated backups for testing
      connected = false;
      Serial.println("   Status: ⚠️  Simulated mode (FIREBASE_ENABLED = false)");
      Serial.println("   Action: Set FIREBASE_ENABLED = true to activate real Firebase");
      Serial.println("   Note: Backup messages will appear every 15 seconds");
      Serial.println();
      delay(500);
      return;
    }

    // Configure Firebase
    config.api_key = FIREBASE_API_KEY;
    config.database_url = FIREBASE_HOST;
    
    // Authentication (REQUIRED for Firebase)
    auth.user.email = FIREBASE_USER_EMAIL;
    auth.user.password = FIREBASE_USER_PASSWORD;
    Serial.println("   Auth: Email/Password");
    
    // Assign callback functions
    config.token_status_callback = tokenStatusCallback;
    
    // Initialize Firebase
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    // Wait for token generation (increased timeout)
    Serial.print("   Connecting");
    int attempts = 0;
    while (!Firebase.ready() && attempts < 40) {  // Increased from 20 to 40
      Serial.print(".");
      delay(500);
      attempts++;
    }
    
    Serial.println();
    
    if (Firebase.ready()) {
      initialized = true;
      connected = true;
      Serial.println("   Status: ✅ Connected to Firebase");
      Serial.println("   Action: Backups will occur every 15 seconds during simulation");
      
      // Save device info on startup
      saveDeviceInfo("v3.0", "RandomForest-250trees");
      updateDeviceStatus(true);
    } else {
      Serial.println("   Status: ❌ Connection failed");
      Serial.println("   Action: Check credentials and network");
      Serial.println("   Help: Verify Firebase library installed and WiFi connected");
      Serial.println("   Note: Make sure Firebase rules allow writes (test with public rules)");
    }
    
    Serial.println();
    delay(500);
  }

  // ==================== DATA BACKUP ====================
  // Backup sensor data and prediction to Firebase
  bool backupData(float temperature, float humidity, float pressure, float lux, 
                  const char* prediction, unsigned long inferenceTime) {
    if (!shouldBackup()) {
      return false;
    }
    
    lastBackupTime = millis();
    totalBackups++;
    readingCount++;
    
    // Generate timestamp (seconds since boot)
    unsigned long timestamp = millis() / 1000;
    
    Serial.println("\n💾 Firebase Backup:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("   Reading #%lu | Timestamp: %lu\n", readingCount, timestamp);
    Serial.printf("   Temperature: %.2f °C\n", temperature);
    Serial.printf("   Humidity: %.2f %%\n", humidity);
    Serial.printf("   Pressure: %.2f Pa\n", pressure);
    Serial.printf("   Lux: %.2f\n", lux);
    Serial.printf("   Prediction: %s\n", prediction);
    Serial.printf("   Inference: %lu µs\n", inferenceTime);
    
    if (!Firebase.ready()) {
      Serial.println("   Status: ❌ Firebase not ready");
      Serial.println("─────────────────────────────────────────────────────────");
      onBackupFailed();
      return false;
    }
    
    // Create path: /devices/{device_id}/readings/{timestamp}
    String basePath = "/devices/" + deviceID + "/readings/" + String(timestamp);
    
    // Create JSON object for the reading
    FirebaseJson json;
    json.set("temperature", temperature);
    json.set("humidity", humidity);
    json.set("pressure", pressure);
    json.set("lux", lux);
    json.set("prediction", prediction);
    json.set("inference_time", inferenceTime);
    json.set("timestamp", timestamp);
    json.set("device_id", deviceID.c_str());
    
    // Upload to Firebase
    if (Firebase.RTDB.setJSON(&fbdo, basePath.c_str(), &json)) {
      Serial.println("   Status: ✅ Backup successful");
      Serial.println("─────────────────────────────────────────────────────────");
      onBackupSuccess();
      return true;
    } else {
      Serial.println("   Status: ❌ Backup failed");
      Serial.printf("   Error: %s\n", fbdo.errorReason().c_str());
      Serial.println("─────────────────────────────────────────────────────────");
      onBackupFailed();
      return false;
    }
  }

  // Backup data with gas sensor readings
  bool backupDataWithGas(float temperature, float humidity, float pressure, float lux,
                         float gasPPM, String gasQuality,
                         const char* prediction, unsigned long inferenceTime) {
    if (!shouldBackup()) {
      return false;
    }
    
    lastBackupTime = millis();
    totalBackups++;
    readingCount++;
    unsigned long timestamp = millis() / 1000;
    
    Serial.println("\n💾 Firebase Backup (with gas):");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("   Reading #%lu | Timestamp: %lu\n", readingCount, timestamp);
    Serial.printf("   Temperature: %.2f °C\n", temperature);
    Serial.printf("   Humidity: %.2f %%\n", humidity);
    Serial.printf("   Pressure: %.2f Pa\n", pressure);
    Serial.printf("   Lux: %.2f\n", lux);
    Serial.printf("   Gas: %.1f PPM (%s)\n", gasPPM, gasQuality.c_str());
    Serial.printf("   Prediction: %s\n", prediction);
    Serial.printf("   Inference: %lu µs\n", inferenceTime);
    
    if (!Firebase.ready()) {
      Serial.println("   Status: ❌ Firebase not ready");
      Serial.println("─────────────────────────────────────────────────────────");
      onBackupFailed();
      return false;
    }
    
    String basePath = "/devices/" + deviceID + "/readings/" + String(timestamp);
    
    FirebaseJson json;
    json.set("temperature", temperature);
    json.set("humidity", humidity);
    json.set("pressure", pressure);
    json.set("lux", lux);
    json.set("gas_ppm", gasPPM);
    json.set("gas_quality", gasQuality.c_str());
    json.set("prediction", prediction);
    json.set("inference_time", inferenceTime);
    json.set("timestamp", timestamp);
    json.set("device_id", deviceID.c_str());
    
    if (Firebase.RTDB.setJSON(&fbdo, basePath.c_str(), &json)) {
      Serial.println("   Status: ✅ Backup successful");
      Serial.println("─────────────────────────────────────────────────────────");
      onBackupSuccess();
      return true;
    } else {
      Serial.println("   Status: ❌ Backup failed");
      Serial.printf("   Error: %s\n", fbdo.errorReason().c_str());
      Serial.println("─────────────────────────────────────────────────────────");
      onBackupFailed();
      return false;
    }
  }

  // ==================== DEVICE MANAGEMENT ====================
  // Save device metadata (called once on startup)
  bool saveDeviceInfo(const char* firmwareVersion, const char* modelType) {
    if (!initialized || !enabled) {
      return false;
    }
    
    Serial.println("\n💾 Saving Device Info:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("   Device ID: %s\n", deviceID.c_str());
    Serial.printf("   Firmware: %s\n", firmwareVersion);
    Serial.printf("   Model: %s\n", modelType);
    Serial.printf("   Chip: %s (%d cores @ %d MHz)\n",
                  ESP.getChipModel(), ESP.getChipCores(), ESP.getCpuFreqMHz());
    Serial.printf("   Flash: %.2f MB\n", ESP.getFlashChipSize() / (1024.0 * 1024.0));
    Serial.printf("   MAC: %s\n", WiFi.macAddress().c_str());
    
    if (!Firebase.ready()) {
      Serial.println("   Status: ❌ Firebase not ready");
      Serial.println("─────────────────────────────────────────────────────────");
      return false;
    }
    
    String basePath = "/devices/" + deviceID + "/info";
    
    FirebaseJson json;
    json.set("device_id", deviceID.c_str());
    json.set("firmware_version", firmwareVersion);
    json.set("model_type", modelType);
    json.set("chip_model", ESP.getChipModel());
    json.set("chip_cores", ESP.getChipCores());
    json.set("cpu_freq_mhz", ESP.getCpuFreqMHz());
    json.set("flash_size_mb", ESP.getFlashChipSize() / (1024.0 * 1024.0));
    json.set("mac_address", WiFi.macAddress().c_str());
    json.set("last_boot", millis() / 1000);
    
    if (Firebase.RTDB.setJSON(&fbdo, basePath.c_str(), &json)) {
      Serial.println("   Status: ✅ Device info saved");
      Serial.println("─────────────────────────────────────────────────────────");
      return true;
    } else {
      Serial.println("   Status: ❌ Failed to save");
      Serial.printf("   Error: %s\n", fbdo.errorReason().c_str());
      Serial.println("─────────────────────────────────────────────────────────");
      return false;
    }
  }

  // Update device status (online/offline)
  void updateDeviceStatus(bool online) {
    if (!initialized || !enabled) {
      return;
    }
    
    if (Firebase.ready()) {
      String path = "/devices/" + deviceID + "/status";
      FirebaseJson json;
      json.set("online", online);
      json.set("last_seen", millis() / 1000);
      Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json);
    }
  }

  // ==================== STATISTICS ====================
  void printStatistics() {
    Serial.println("\n📊 Firebase Statistics:");
    Serial.println("─────────────────────────────────────────────────────────");
    Serial.printf("   Total Backups: %lu\n", totalBackups);
    Serial.printf("   Successful: %lu (%.1f%%)\n", 
                  successfulBackups,
                  totalBackups > 0 ? (successfulBackups * 100.0 / totalBackups) : 0);
    Serial.printf("   Failed: %lu (%.1f%%)\n",
                  failedBackups,
                  totalBackups > 0 ? (failedBackups * 100.0 / totalBackups) : 0);
    Serial.printf("   Consecutive Failures: %lu\n", consecutiveFailures);
    if (consecutiveFailures >= maxConsecutiveFailures) {
      Serial.println("   Status: ⚠️  Backup temporarily disabled (too many failures)");
    }
    Serial.println("─────────────────────────────────────────────────────────");
  }

  // ==================== UTILITY FUNCTIONS ====================
  // Generate unique device ID from MAC address
  String generateDeviceID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char deviceIDStr[18];
    sprintf(deviceIDStr, "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(deviceIDStr);
  }

  // Check if ready for backup
  bool shouldBackup() {
    if (!enabled || !initialized) {
      return false;
    }
    
    // Stop trying after too many failures
    if (consecutiveFailures >= maxConsecutiveFailures) {
      return false;
    }
    
    // Don't backup too frequently
    return (millis() - lastBackupTime >= backupInterval);
  }

  // Success callback
  void onBackupSuccess() {
    successfulBackups++;
    consecutiveFailures = 0;
  }

  // Failure callback
  void onBackupFailed() {
    failedBackups++;
    consecutiveFailures++;
    if (consecutiveFailures >= maxConsecutiveFailures) {
      Serial.println("\n⚠️  Too many consecutive backup failures!");
      Serial.println("   Firebase backup temporarily disabled.");
      Serial.println("   Check Firebase configuration and WiFi connection.");
    }
  }

  // Getters
  String getDeviceID() { return deviceID; }
  bool isEnabled() { return enabled; }
  bool isConnected() { return connected; }
  unsigned long getTotalBackups() { return totalBackups; }
  unsigned long getSuccessfulBackups() { return successfulBackups; }
  unsigned long getFailedBackups() { return failedBackups; }

  // Token status callback (for Firebase authentication)
  static void tokenStatusCallback(TokenInfo info) {
    if (info.status == token_status_error) {
      Serial.printf("⚠️  Token error: %s\n", info.error.message.c_str());
    } else if (info.status == token_status_ready) {
      Serial.println("✅ Token ready");
    }
  }
};

#endif // FIREBASE_MANAGER_H
