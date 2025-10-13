/*
 * Cloud Manager - ThingSpeak Integration
 * 
 * Handles ThingSpeak connection testing and data upload
 */

#ifndef CLOUD_MANAGER_H
#define CLOUD_MANAGER_H

#include <HTTPClient.h>
#include <WiFi.h>

class CloudManager {
private:
    String apiKey;
    String channelId;
    bool connected;
    
public:
    CloudManager(const char* apiKey, const char* channelId) 
        : apiKey(apiKey), channelId(channelId), connected(false) {}
    
    bool testConnection() {
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("STEP 2: ThingSpeak Connection");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("   ⚠️  Skipped (WiFi not connected)");
            Serial.println();
            return false;
        }
        
        // Test DNS resolution first
        Serial.println("   Testing DNS resolution...");
        IPAddress serverIP;
        if (WiFi.hostByName("api.thingspeak.com", serverIP)) {
            Serial.printf("   ✅ DNS resolved: %s\n", serverIP.toString().c_str());
        } else {
            Serial.println("   ❌ DNS resolution FAILED!");
            Serial.println("   Your router may not have internet access");
            Serial.println();
            return false;
        }
        
        Serial.println("   Testing HTTP connection...");
        
        HTTPClient http;
        // Just test if ThingSpeak API is reachable (don't send data yet)
        // Using the channel read endpoint to avoid rate limit issues
        String url = "http://api.thingspeak.com/channels/" + channelId + "/status.json";
        
        http.begin(url);
        http.setReuse(false);  // CRITICAL: Disable connection reuse to prevent timing issues
        http.setTimeout(10000);
        
        int httpCode = http.GET();
        
        if (httpCode > 0) {
            Serial.printf("   Response: HTTP %d\n", httpCode);
            
            if (httpCode == 200 || httpCode == 404) {
                // 200 = channel public, 404 = channel private (both mean API works!)
                connected = true;
                Serial.println("   ✅ ThingSpeak API Reachable!");
                Serial.printf("   Channel: %s\n", channelId.c_str());
                Serial.println("   Ready to upload data");
                Serial.println();
                Serial.println("   ℹ️  Note: Free tier allows updates every 15 seconds");
            } else {
                Serial.println("   ⚠️  Unexpected API response");
                Serial.println("   System will continue, upload may fail");
                connected = true; // Still try to upload
            }
        } else {
            Serial.printf("   ❌ Connection failed: %s\n", http.errorToString(httpCode).c_str());
            Serial.println("   Possible causes:");
            Serial.println("      • Router has no internet access");
            Serial.println("      • DNS cannot resolve api.thingspeak.com");
            Serial.println("      • Firewall blocking outbound HTTP");
            Serial.println("   System will continue WITHOUT cloud upload");
        }
        
        http.end();
        Serial.println();
        delay(1000);
        
        return connected;
    }
    
    bool uploadData(float temp, float humid, float pressure, float lux, float gas) {
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("☁️  Uploading to ThingSpeak...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // CRITICAL: Revalidate WiFi connection state before upload
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("   ❌ WiFi not connected! Skipping upload.");
            Serial.println();
            return false;
        }
        
        // CRITICAL: Re-test DNS resolution before upload (ESP32 DNS can fail intermittently)
        Serial.println("   🔍 Validating connection...");
        IPAddress serverIP;
        if (!WiFi.hostByName("api.thingspeak.com", serverIP)) {
            Serial.println("   ❌ DNS resolution failed!");
            Serial.println("   Possible causes:");
            Serial.println("      • Router lost internet connection");
            Serial.println("      • DNS server temporarily unavailable");
            Serial.println("   Waiting 2 seconds before retry...");
            Serial.println();
            delay(2000);
            return false;
        }
        Serial.printf("   ✅ DNS OK: %s\n", serverIP.toString().c_str());
        
        // Build ThingSpeak URL
        String url = "http://api.thingspeak.com/update?api_key=" + apiKey;
        url += "&field1=" + String(temp, 2);
        url += "&field2=" + String(humid, 2);
        url += "&field3=" + String(pressure/100.0, 2);
        url += "&field4=" + String(lux, 2);
        url += "&field5=" + String(gas, 0);
        
        Serial.println("   📡 Sending data...");
        
        HTTPClient http;
        http.begin(url);
        http.setReuse(false);  // CRITICAL: Disable connection reuse to prevent ESP32 HTTP issues
        http.setTimeout(10000);
        
        int httpCode = http.GET();
        
        bool success = false;
        
        if (httpCode > 0) {
            Serial.printf("   📥 Response: HTTP %d\n", httpCode);
            
            if (httpCode == 200) {
                String response = http.getString();
                Serial.println("   ✅ Data uploaded successfully!");
                Serial.printf("   Entry ID: %s\n", response.c_str());
                success = true;
            } else {
                Serial.printf("   ⚠️  Unexpected response code: %d\n", httpCode);
            }
        } else {
            Serial.printf("   ❌ Connection failed: %s\n", http.errorToString(httpCode).c_str());
            Serial.println("   Possible causes:");
            Serial.println("      • No internet access (check router)");
            Serial.println("      • ThingSpeak server down");
            Serial.println("      • DNS resolution failed");
            Serial.println("      • Firewall blocking port 80");
            Serial.println("      • HTTP client not fully released from previous request");
        }
        
        http.end();
        Serial.println();
        
        return success;
    }
    
    bool isConnected() {
        return connected;
    }
    
    // Upload with retry logic and exponential backoff
    bool uploadWithRetry(float temp, float humid, float pressure, float lux, float gas) {
        const int MAX_RETRIES = 3;
        
        for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
            if (attempt > 1) {
                Serial.printf("   🔄 Retry attempt %d/%d...\n", attempt, MAX_RETRIES);
            }
            
            // Attempt upload
            bool success = uploadData(temp, humid, pressure, lux, gas);
            
            if (success) {
                if (attempt > 1) {
                    Serial.printf("   ✅ Upload succeeded on attempt %d\n", attempt);
                    Serial.println();
                }
                return true;
            }
            
            // Failed - wait before retry (exponential backoff)
            if (attempt < MAX_RETRIES) {
                unsigned long backoffDelay = 2000 * attempt;  // 2s, 4s, 6s
                Serial.printf("   ⏳ Waiting %lu seconds before retry...\n", backoffDelay / 1000);
                Serial.println();
                delay(backoffDelay);
            }
        }
        
        Serial.println("   ❌ All upload attempts failed!");
        Serial.println("   System will continue, data not uploaded to cloud.");
        Serial.println();
        return false;
    }
};

#endif
