/*
 * WiFi Manager Module
 * Handles WiFi connection, reconnection, status monitoring, and LED indicators
 * 
 * Features:
 * - Auto-connect with retry logic
 * - Connection status monitoring
 * - LED status indicators
 * - Connection logging
 * - Network diagnostics
 * 
 * LED Indicators:
 * - WHITE pulsing: Attempting connection
 * - GREEN solid: Connected
 * - RED blinking: Connection failed
 * - YELLOW: Reconnecting
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

// WiFi Configuration
#define WIFI_SSID     "COMFRI"
#define WIFI_PASSWORD "1234567890"

// Connection settings
#define WIFI_TIMEOUT_MS 20000           // 20 seconds timeout per attempt
#define WIFI_RETRY_DELAY_MS 5000        // 5 seconds between retries
#define MAX_WIFI_RETRIES 5              // Maximum connection attempts
#define WIFI_CHECK_INTERVAL 10000       // Check connection every 10 seconds

// WiFi Status enum
enum WiFiStatus {
    WIFI_IDLE,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    WIFI_FAILED,
    WIFI_RECONNECTING
};

class WiFiManager {
private:
    WiFiStatus currentStatus;
    int retryCount;
    unsigned long lastConnectionAttempt;
    unsigned long lastStatusCheck;
    unsigned long connectionStartTime;
    bool autoReconnect;
    
    // Connection statistics
    unsigned int totalConnectionAttempts;
    unsigned int successfulConnections;
    unsigned int failedConnections;
    unsigned long totalConnectedTime;
    unsigned long lastConnectedTime;
    
    // LED reference (external LED controller)
    void (*ledCallback)(uint32_t color) = nullptr;
    
public:
    WiFiManager() {
        currentStatus = WIFI_IDLE;
        retryCount = 0;
        lastConnectionAttempt = 0;
        lastStatusCheck = 0;
        connectionStartTime = 0;
        autoReconnect = true;
        
        totalConnectionAttempts = 0;
        successfulConnections = 0;
        failedConnections = 0;

        totalConnectedTime = 0;
        lastConnectedTime = 0;
    }
    
    // Initialize WiFi manager
    void begin(void (*ledFunction)(uint32_t) = nullptr) {
        ledCallback = ledFunction;
        
        Serial.println("📡 WiFi Manager initialized");
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("   SSID: %s\n", WIFI_SSID);
        Serial.printf("   Auto-reconnect: %s\n", autoReconnect ? "Enabled" : "Disabled");
        Serial.printf("   Max retries: %d\n", MAX_WIFI_RETRIES);
        Serial.println("─────────────────────────────────────────────────────────");
        
        // Clean WiFi state first
        WiFi.disconnect(true);  // Disconnect and clear old settings
        delay(100);
        
        // Set WiFi mode
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(false); // We handle reconnection manually
        delay(100);
        
        Serial.println("   Status: Ready");
        Serial.println();
    }
    
    // Start connection attempt
    bool connect() {
        if (currentStatus == WIFI_CONNECTING) {
            Serial.println("⚠️  Connection already in progress...");
            return false;
        }
        
        Serial.println("╔════════════════════════════════════════════════════════╗");
        Serial.println("║           WIFI CONNECTION ATTEMPT                      ║");
        Serial.println("╚════════════════════════════════════════════════════════╝");
        Serial.println();
        
        currentStatus = WIFI_CONNECTING;
        retryCount = 0;
        totalConnectionAttempts++;
        connectionStartTime = millis();
        
        return attemptConnection();
    }
    
    // Attempt single connection
    bool attemptConnection() {
        retryCount++;
        lastConnectionAttempt = millis();
        
        Serial.printf("📡 Attempt %d/%d: Connecting to '%s'...\n", 
                     retryCount, MAX_WIFI_RETRIES, WIFI_SSID);
        
        // LED: White pulsing (connecting)
        if (ledCallback) ledCallback(0xFFFFFF); // WHITE
        
        // Disconnect any existing connection first
        WiFi.disconnect();
        delay(100);
        
        // Start fresh connection
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        // Wait for connection with timeout
        unsigned long startAttempt = millis();
        int dots = 0;
        
        while (WiFi.status() != WL_CONNECTED && 
               (millis() - startAttempt) < WIFI_TIMEOUT_MS) {
            delay(500);
            Serial.print(".");
            dots++;
            if (dots % 40 == 0) Serial.println();
            
            // Yield to watchdog and other tasks - CRITICAL for ESP32-S3
            yield();
            
            // LED feedback during connection
            if (ledCallback && dots % 2 == 0) {
                ledCallback(0xFFFFFF); // WHITE
            } else if (ledCallback) {
                ledCallback(0x000000); // OFF
            }
        }
        Serial.println();
        
        // Check connection result
        if (WiFi.status() == WL_CONNECTED) {
            onConnectionSuccess();
            return true;
        } else {
            onConnectionFailed();
            return false;
        }
    }
    
    // Connection succeeded
    void onConnectionSuccess() {
        currentStatus = WIFI_CONNECTED;
        successfulConnections++;
        lastConnectedTime = millis();
        
        // LED: Green solid (connected)
        if (ledCallback) ledCallback(0x00FF00); // GREEN
        
        Serial.println("╔════════════════════════════════════════════════════════╗");
        Serial.println("║           ✅ WIFI CONNECTED!                          ║");
        Serial.println("╚════════════════════════════════════════════════════════╝");
        Serial.println();
        Serial.println("📶 Connection Details:");
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("   SSID:          %s\n", WiFi.SSID().c_str());
        Serial.printf("   IP Address:    %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("   Gateway:       %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("   Subnet Mask:   %s\n", WiFi.subnetMask().toString().c_str());
        Serial.printf("   DNS:           %s\n", WiFi.dnsIP().toString().c_str());
        Serial.printf("   MAC Address:   %s\n", WiFi.macAddress().c_str());
        Serial.printf("   Signal (RSSI): %d dBm ", WiFi.RSSI());
        Serial.println(getSignalQuality());
        Serial.printf("   Channel:       %d\n", WiFi.channel());
        Serial.printf("   Connection #:  %u\n", successfulConnections);
        Serial.printf("   Time taken:    %.2f seconds\n", 
                     (millis() - connectionStartTime) / 1000.0f);
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.println();
    }
    
    // Connection failed
    void onConnectionFailed() {
        wl_status_t status = WiFi.status();
        
        Serial.println("✗ Connection failed!");
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("   WiFi Status Code: %d\n", status);
        Serial.printf("   Status Name: %s\n", getWiFiStatusString(status));
        Serial.printf("   Reason: %s\n", getDisconnectReason());
        Serial.println("─────────────────────────────────────────────────────────");
        
        // Additional diagnostics
        Serial.println("\n🔍 Diagnostics:");
        Serial.printf("   WiFi Mode: %s\n", WiFi.getMode() == WIFI_STA ? "Station" : "Other");
        Serial.printf("   SSID Length: %d characters\n", strlen(WIFI_SSID));
        Serial.printf("   Password Length: %d characters\n", strlen(WIFI_PASSWORD));
        Serial.printf("   MAC Address: %s\n", WiFi.macAddress().c_str());
        Serial.println();
        
        if (retryCount < MAX_WIFI_RETRIES) {
            currentStatus = WIFI_RECONNECTING;
            Serial.printf("   ⏳ Retrying in %d seconds... (Attempt %d/%d)\n", 
                         WIFI_RETRY_DELAY_MS/1000, retryCount + 1, MAX_WIFI_RETRIES);
            Serial.println();
            
            // LED: Yellow (reconnecting)
            if (ledCallback) ledCallback(0xFFFF00); // YELLOW
            
            delay(WIFI_RETRY_DELAY_MS);
            attemptConnection();
        } else {
            currentStatus = WIFI_FAILED;
            failedConnections++;
            
            // LED: Red blinking (failed)
            if (ledCallback) ledCallback(0xFF0000); // RED
            
            Serial.println("╔════════════════════════════════════════════════════════╗");
            Serial.println("║           ❌ WIFI CONNECTION FAILED                   ║");
            Serial.println("╚════════════════════════════════════════════════════════╝");
            Serial.println();
            Serial.printf("   All %d connection attempts failed.\n", MAX_WIFI_RETRIES);
            Serial.println("\n   Troubleshooting:");
            Serial.println("   1. Check WiFi credentials:");
            Serial.printf("      - SSID: '%s'\n", WIFI_SSID);
            Serial.printf("      - Password: %d characters (hidden)\n", strlen(WIFI_PASSWORD));
            Serial.println("   2. Router is powered on and in range");
            Serial.println("   3. Network is 2.4GHz (ESP32 doesn't support 5GHz)");
            Serial.println("   4. Check router MAC filtering");
            Serial.println("   5. Try power-cycling the router");
            Serial.println();
            Serial.println("   Type 'reconnect' to try again.");
            Serial.println();
        }
    }
    
    // Monitor connection status (call in loop)
    void update() {
        // Only check periodically
        if (millis() - lastStatusCheck < WIFI_CHECK_INTERVAL) {
            return;
        }
        lastStatusCheck = millis();
        
        // Check if we were connected but now disconnected
        if (currentStatus == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED) {
            Serial.println("\n⚠️  WiFi connection lost!");
            onDisconnected();
        }
        
        // Auto-reconnect if enabled
        if (autoReconnect && currentStatus == WIFI_DISCONNECTED) {
            Serial.println("🔄 Auto-reconnecting...");
            connect();
        }
    }
    
    // Handle disconnection
    void onDisconnected() {
        if (currentStatus == WIFI_CONNECTED) {
            // Update connected time
            totalConnectedTime += (millis() - lastConnectedTime);
        }
        
        currentStatus = WIFI_DISCONNECTED;
        
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.printf("   Reason: %s\n", getDisconnectReason());
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.println();
        
        // LED: Red blinking (disconnected)
        if (ledCallback) ledCallback(0xFF0000); // RED
    }
    
    // Get connection status
    WiFiStatus getStatus() {
        return currentStatus;
    }
    
    // Check if connected
    bool isConnected() {
        return (currentStatus == WIFI_CONNECTED && WiFi.status() == WL_CONNECTED);
    }
    
    // Get IP address
    String getIPAddress() {
        if (isConnected()) {
            return WiFi.localIP().toString();
        }
        return "Not connected";
    }
    
    // Get signal strength description
    const char* getSignalQuality() {
        int rssi = WiFi.RSSI();
        if (rssi > -50) return "📶 Excellent";
        if (rssi > -60) return "📶 Good";
        if (rssi > -70) return "📶 Fair";
        return "📶 Weak";
    }
    
    // Get WiFi status as string
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
    
    // Get disconnect reason
    const char* getDisconnectReason() {
        switch (WiFi.status()) {
            case WL_NO_SSID_AVAIL: return "Network not found";
            case WL_CONNECT_FAILED: return "Wrong password or authentication failed";
            case WL_CONNECTION_LOST: return "Connection lost (weak signal or router restart)";
            case WL_DISCONNECTED: return "Disconnected";
            default: return "Unknown reason";
        }
    }
    
    // Disconnect WiFi
    void disconnect() {
        if (currentStatus == WIFI_CONNECTED) {
            totalConnectedTime += (millis() - lastConnectedTime);
        }
        
        WiFi.disconnect(true);
        currentStatus = WIFI_DISCONNECTED;
        Serial.println("📡 WiFi disconnected");
    }
    
    // Enable/disable auto-reconnect
    void setAutoReconnect(bool enable) {
        autoReconnect = enable;
        Serial.printf("🔄 Auto-reconnect: %s\n", enable ? "Enabled" : "Disabled");
    }
    
    // Get status as string
    const char* getStatusString() {
        switch (currentStatus) {
            case WIFI_IDLE: return "Idle";
            case WIFI_CONNECTING: return "Connecting...";
            case WIFI_CONNECTED: return "Connected ✅";
            case WIFI_DISCONNECTED: return "Disconnected";
            case WIFI_FAILED: return "Failed ❌";
            case WIFI_RECONNECTING: return "Reconnecting...";
            default: return "Unknown";
        }
    }
    
    // Scan for available networks
    void scanNetworks() {
        Serial.println("\n📡 Scanning for WiFi networks...");
        Serial.println("─────────────────────────────────────────────────────────");
        
        int n = WiFi.scanNetworks();
        
        if (n == 0) {
            Serial.println("   No networks found");
        } else {
            Serial.printf("   Found %d networks:\n\n", n);
            for (int i = 0; i < n; i++) {
                Serial.printf("   %2d: %-32s %3d dBm [%s] %s\n",
                             i + 1,
                             WiFi.SSID(i).c_str(),
                             WiFi.RSSI(i),
                             WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured",
                             WiFi.SSID(i) == WIFI_SSID ? "← Configured" : "");
            }
        }
        Serial.println("─────────────────────────────────────────────────────────");
        Serial.println();
    }
};

#endif // WIFI_MANAGER_H
