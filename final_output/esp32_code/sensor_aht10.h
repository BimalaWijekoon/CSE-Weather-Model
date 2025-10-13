/*
 * AHT10 Temperature & Humidity Sensor
 * 
 * I2C Address: 0x38
 * Note: Wire.begin() must be called in main code before using this sensor
 */

#ifndef SENSOR_AHT10_H
#define SENSOR_AHT10_H

#include <Adafruit_AHTX0.h>

class AHT10Sensor {
private:
    Adafruit_AHTX0 aht;
    bool available;
    float lastTemp;
    float lastHumid;
    
public:
    AHT10Sensor() : available(false), lastTemp(0), lastHumid(0) {}
    
    bool begin() {
        Serial.print("   [AHT10] Initializing (0x38)... ");
        
        if (aht.begin()) {
            available = true;
            Serial.println("✅ Connected");
            return true;
        } else {
            available = false;
            Serial.println("❌ Not found");
            return false;
        }
    }
    
    void read() {
        if (!available) {
            // Use fallback values if sensor not available
            lastTemp = 25.0;
            lastHumid = 50.0;
            return;
        }
        
        sensors_event_t humidity_event, temp_event;
        aht.getEvent(&humidity_event, &temp_event);
        
        lastTemp = temp_event.temperature;
        lastHumid = humidity_event.relative_humidity;
    }
    
    float getTemperature() {
        return lastTemp;
    }
    
    float getHumidity() {
        return lastHumid;
    }
    
    bool isAvailable() {
        return available;
    }
    
    void printReading() {
        Serial.printf("   🌡️  Temp: %.1f°C | 💧 Humid: %.1f%%", lastTemp, lastHumid);
        if (!available) Serial.print(" [SIMULATED]");
        Serial.println();
    }
};

#endif
