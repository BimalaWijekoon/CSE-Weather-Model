/*
 * BME280 Pressure Sensor
 * 
 * I2C Address: 0x76 (SDO→GND) or 0x77 (SDO→VCC)
 * Note: Wire.begin() must be called in main code before using this sensor
 */

#ifndef SENSOR_BME280_H
#define SENSOR_BME280_H

#include <Adafruit_BME280.h>

class BME280Sensor {
private:
    Adafruit_BME280 bme;
    bool available;
    float lastPressure;
    
public:
    BME280Sensor() : available(false), lastPressure(0) {}
    
    bool begin() {
        Serial.print("   [BME280] Initializing (0x76/0x77)... ");
        
        // Try 0x76 first, then 0x77
        if (bme.begin(0x76) || bme.begin(0x77)) {
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
            lastPressure = 101325.0;  // Standard atmospheric pressure
            return;
        }
        
        lastPressure = bme.readPressure();
    }
    
    float getPressure() {
        return lastPressure;
    }
    
    float getPressureHPa() {
        return lastPressure / 100.0;
    }
    
    bool isAvailable() {
        return available;
    }
    
    void printReading() {
        Serial.printf("   🌀  Pressure: %.0f Pa (%.2f hPa)", lastPressure, lastPressure/100.0);
        if (!available) Serial.print(" [SIMULATED]");
        Serial.println();
    }
};

#endif
