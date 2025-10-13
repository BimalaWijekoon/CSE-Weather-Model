/*
 * Sensor Test Module
 * 
 * Handles real sensor testing with 15-second data collection and averaging
 * Includes:
 * - I2C bus initialization
 * - Sensor discovery and initialization
 * - Multi-sample reading (15 readings over 15 seconds)
 * - Data averaging
 * - ML prediction
 * - Cloud upload
 */

#ifndef SENSOR_TEST_H
#define SENSOR_TEST_H

#include <Wire.h>
#include "weather_model_250.h"
#include "weather_scaling.h"
#include "sensor_aht10.h"
#include "sensor_bme280.h"
#include "sensor_bh1750.h"
#include "sensor_mq2.h"
#include "cloud_manager.h"
#include "firebase_manager.h"

class SensorTest {
private:
    // I2C Configuration
    int sdaPin;
    int sclPin;
    
    // Sensor objects
    AHT10Sensor* aht10;
    BME280Sensor* bme280;
    BH1750Sensor* bh1750;
    MQ2Sensor* mq2;
    
    // Managers
    CloudManager* cloudManager;
    FirebaseManager* firebaseManager;
    
    // ML Classifier
    Eloquent::ML::Port::RandomForest* classifier;
    
    // State
    bool sensorsInitialized;
    
    // Weather classes
    const char* weatherClasses[5] = {"Cloudy", "Foggy", "Rainy", "Stormy", "Sunny"};
    const char* weatherEmojis[5] = {"☁️", "🌫️", "🌧️", "⛈️", "☀️"};
    
public:
    SensorTest(int sda, int scl, 
               AHT10Sensor* aht, BME280Sensor* bme, BH1750Sensor* bh, MQ2Sensor* mq,
               CloudManager* cloud, FirebaseManager* firebase,
               Eloquent::ML::Port::RandomForest* clf) 
        : sdaPin(sda), sclPin(scl), 
          aht10(aht), bme280(bme), bh1750(bh), mq2(mq),
          cloudManager(cloud), firebaseManager(firebase),
          classifier(clf), sensorsInitialized(false) {}
    
    // Run complete sensor test
    void run() {
        Serial.println("\n╔════════════════════════════════════════════════════════╗");
        Serial.println("║           SENSOR TEST & PREDICTION MODE               ║");
        Serial.println("╚════════════════════════════════════════════════════════╝");
        Serial.println();
        
        // Initialize sensors if not already done
        if (!sensorsInitialized) {
            initializeSensors();
        }
        
        // Collect 15 readings
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("Collecting 15 readings (1-second intervals)...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println();
        
        float tempSum = 0, humidSum = 0, pressureSum = 0, luxSum = 0, gasSum = 0;
        int validReadings = 0;
        
        for (int i = 1; i <= 15; i++) {
            Serial.printf("📊 Reading #%d/15\n", i);
            Serial.println("──────────────────────────────────────────────");
            
            // Read sensors
            float temp, humid, pressure, lux, gas;
            readSensors(temp, humid, pressure, lux, gas);
            
            tempSum += temp;
            humidSum += humid;
            pressureSum += pressure;
            luxSum += lux;
            gasSum += gas;
            validReadings++;
            
            Serial.println();
            
            if (i < 15) {
                delay(1000);
            }
        }
        
        // Calculate averages
        float avgTemp = tempSum / validReadings;
        float avgHumid = humidSum / validReadings;
        float avgPressure = pressureSum / validReadings;
        float avgLux = luxSum / validReadings;
        float avgGas = gasSum / validReadings;
        
        // Display averages
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("Averaged Sensor Data (15 readings over 15 seconds)");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.printf("   🌡️  Temperature: %.2f °C\n", avgTemp);
        Serial.printf("   💧 Humidity:    %.2f %%\n", avgHumid);
        Serial.printf("   🌀  Pressure:    %.2f hPa\n", avgPressure / 100.0);
        Serial.printf("   💡 Light:       %.2f lux\n", avgLux);
        Serial.printf("   🌫️  Gas (LPG):   %.2f PPM\n", avgGas);
        Serial.println();
        
        // Make prediction
        const char* prediction = makePrediction(avgTemp, avgHumid, avgPressure, avgLux);
        
        // Upload to cloud with retry logic
        if (cloudManager->isConnected()) {
            cloudManager->uploadWithRetry(avgTemp, avgHumid, avgPressure, avgLux, avgGas);
        }
        
        // Backup to Firebase (note: Firebase uses backupDataWithGas for gas sensor data)
        // For now, use basic backupData since gas sensor not used in model
        unsigned long inferenceTime = 0; // TODO: Get actual inference time from prediction
        firebaseManager->backupData(avgTemp, avgHumid, avgPressure, avgLux, prediction, inferenceTime);
        
        Serial.println("\n✅ Test complete! Type 'sensortest' to run again.\n");
    }
    
    // Check if sensors are initialized
    bool isInitialized() {
        return sensorsInitialized;
    }
    
private:
    // Initialize I2C and all sensors
    void initializeSensors() {
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("Initializing I2C and Sensors...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println();
        
        // Activate I2C bus
        Serial.println("   🔌 Activating I2C Bus (connecting sensors):");
        Serial.printf("   • SDA: GPIO %d (was high-impedance, now I2C)\n", sdaPin);
        Serial.printf("   • SCL: GPIO %d (was high-impedance, now I2C)\n", sclPin);
        Serial.println("   • Sensors were invisible during WiFi startup");
        Serial.println();
        
        Wire.setPins(sdaPin, sclPin);
        Wire.begin();
        Wire.setClock(50000);  // 50kHz - Reduced speed for better WiFi coexistence and noise immunity
        delay(500);
        Serial.println("   ✅ I2C bus active - sensors now connected!");
        Serial.println();
        
        // Scan I2C bus
        Serial.println("   Scanning I2C bus...");
        scanI2C();
        Serial.println();
        
        // Initialize all sensors
        Serial.println("   Initializing sensors...");
        aht10->begin();
        delay(300);
        
        bme280->begin();
        delay(300);
        
        bh1750->begin();
        delay(300);
        
        mq2->begin();
        
        Serial.println();
        sensorsInitialized = true;
    }
    
    // Scan I2C bus for devices
    void scanI2C() {
        int found = 0;
        for (byte addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("     • 0x%02X detected", addr);
                if (addr == 0x23) Serial.print(" (BH1750)");
                else if (addr == 0x38) Serial.print(" (AHT10)");
                else if (addr == 0x76 || addr == 0x77) Serial.print(" (BME280)");
                Serial.println();
                found++;
            }
        }
        if (found == 0) {
            Serial.println("     ⚠️  No I2C devices found!");
        }
    }
    
    // Read all sensors
    void readSensors(float &temp, float &humid, float &pressure, float &lux, float &gas) {
        // Read all sensors
        aht10->read();
        bme280->read();
        bh1750->read();
        mq2->read();
        
        // Get values
        temp = aht10->getTemperature();
        humid = aht10->getHumidity();
        pressure = bme280->getPressure();
        lux = bh1750->getLux();
        gas = mq2->getPPM();
        
        // Print readings
        aht10->printReading();
        bme280->printReading();
        bh1750->printReading();
        mq2->printReading();
    }
    
    // Make ML prediction
    const char* makePrediction(float temp, float humid, float pressure, float lux) {
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("🔮 Weather Prediction");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // Scale features
        float scaled[4];
        scale_features(temp, humid, pressure, lux, scaled);
        
        // Predict
        unsigned long startTime = micros();
        int predictedClass = classifier->predict(scaled);
        unsigned long inferenceTime = micros() - startTime;
        
        const char* weather = weatherClasses[predictedClass];
        
        Serial.printf("   Prediction: %s %s\n", weatherEmojis[predictedClass], weather);
        Serial.printf("   Inference Time: %lu µs (%.3f ms)\n", inferenceTime, inferenceTime/1000.0);
        Serial.println();
        
        return weather;
    }
};

#endif // SENSOR_TEST_H
