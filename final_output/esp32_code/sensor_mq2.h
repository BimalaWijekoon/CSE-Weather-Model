/*
 * MQ2 Gas Sensor (Analog)
 * 
 * GPIO Pin: 6 (ADC1_CH5)
 * Note: This is an analog sensor, no I2C required
 */

#ifndef SENSOR_MQ2_H
#define SENSOR_MQ2_H

#include <Arduino.h>

class MQ2Sensor {
private:
    int pin;
    float lastPPM;
    
public:
    MQ2Sensor(int pin = 6) : pin(pin), lastPPM(0) {}
    
    void begin() {
        Serial.printf("   [MQ2] Initializing (GPIO %d)... ", pin);
        pinMode(pin, INPUT);
        Serial.println("✅ Ready");
    }
    
    void read() {
        int rawValue = analogRead(pin);
        // Simple linear mapping (can be calibrated for better accuracy)
        lastPPM = map(rawValue, 0, 4095, 200, 10000);
    }
    
    float getPPM() {
        return lastPPM;
    }
    
    int getRawValue() {
        return analogRead(pin);
    }
    
    const char* getAirQuality() {
        if (lastPPM < 1000) return "Good";
        else if (lastPPM < 2000) return "Moderate";
        else if (lastPPM < 5000) return "Poor";
        else return "Hazardous";
    }
    
    const char* getEmoji() {
        if (lastPPM < 1000) return "✅";
        else if (lastPPM < 2000) return "⚠️";
        else if (lastPPM < 5000) return "🔶";
        else return "🔴";
    }
    
    void printReading() {
        Serial.printf("   🌫️  Gas: %.0f PPM (%s) %s\n", 
                      lastPPM, getAirQuality(), getEmoji());
    }
};

#endif
