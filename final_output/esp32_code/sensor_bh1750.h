/*
 * BH1750 Light Sensor
 * 
 * I2C Address: 0x23 (ADDR→GND) or 0x5C (ADDR→VCC)
 * Note: Wire.begin() must be called in main code before using this sensor
 */

#ifndef SENSOR_BH1750_H
#define SENSOR_BH1750_H

#include <BH1750.h>

class BH1750Sensor {
private:
    BH1750 lightMeter;
    bool available;
    float lastLux;
    
public:
    BH1750Sensor() : available(false), lastLux(0) {}
    
    bool begin() {
        Serial.print("   [BH1750] Initializing (0x23)... ");
        
        if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
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
            // Use fallback value if sensor not available
            lastLux = 500.0;  // Typical indoor lighting
            return;
        }
        
        lastLux = lightMeter.readLightLevel();
    }
    
    float getLux() {
        return lastLux;
    }
    
    const char* getLightCondition() {
        if (lastLux < 10) return "Dark";
        else if (lastLux < 100) return "Dim";
        else if (lastLux < 500) return "Indoor";
        else if (lastLux < 1000) return "Bright";
        else return "Very Bright";
    }
    
    bool isAvailable() {
        return available;
    }
    
    void printReading() {
        Serial.printf("   💡 Light: %.1f lux (%s)", lastLux, getLightCondition());
        if (!available) Serial.print(" [SIMULATED]");
        Serial.println();
    }
};

#endif
