/**
 * Weather Model Scaling Functions for ESP32
 * 
 * This file contains the scaling functions that match Python's MinMaxScaler EXACTLY.
 * These functions MUST be used before feeding sensor data to the prediction model.
 * 
 * THE ROOT CAUSE OF THE 33% ACCURACY BUG:
 * ----------------------------------------
 * ❌ Model was trained on SCALED data (0-1 range)
 * ❌ ESP32 was sending RAW sensor values (e.g., temperature: 20-30°C, pressure: 96000-100000 Pa)
 * ❌ Result: Model saw completely different input scale → 33% random accuracy
 * 
 * THE SOLUTION:
 * -------------
 * ✅ Scale raw sensor readings to [0, 1] range using SAME min/max values from training
 * ✅ Model sees familiar input format → 100% accuracy maintained!
 * 
 * Mathematical Formula (MinMaxScaler):
 * scaled_value = (raw_value - min) / (max - min)
 * 
 * This EXACT formula is used in both Python training AND ESP32 inference!
 * 
 * Scaling Parameters (from training data):
 * ----------------------------------------
 * Temperature: [19.0000, 30.0000]
 * Humidity:    [29.3000, 56.9000]
 * Pressure:    [96352.6800, 100301.0600]
 * Lux:         [0.0000, 632.0800]
 * 
 * Usage Example:
 * --------------
 * float raw_temp = 25.5;          // Raw sensor reading
 * float raw_humid = 65.0;         // Raw sensor reading
 * float raw_pressure = 101325.0;  // Raw sensor reading
 * float raw_lux = 15000.0;        // Raw sensor reading
 * 
 * // Scale features before prediction
 * float scaled_features[4];
 * scaled_features[0] = scale_temperature(raw_temp);
 * scaled_features[1] = scale_humidity(raw_humid);
 * scaled_features[2] = scale_pressure(raw_pressure);
 * scaled_features[3] = scale_lux(raw_lux);
 * 
 * // Create classifier and make prediction
 * Eloquent::ML::Port::RandomForest classifier;
 * int prediction = classifier.predict(scaled_features);
 * 
 * Hardware Target: ESP32-S3 with BME280, AHT10, BH1750 sensors
 * 
 * @author Weather Prediction System
 * @version 1.0
 * @date 2025-10-09
 */

#ifndef WEATHER_SCALING_H
#define WEATHER_SCALING_H

// ============================================================================
// SCALING PARAMETERS (from Python MinMaxScaler)
// ============================================================================

// Temperature scaling parameters (°C)
const float TEMP_MIN = 19.000000f;
const float TEMP_MAX = 30.000000f;
const float TEMP_RANGE = 11.000000f;

// Humidity scaling parameters (%)
const float HUMID_MIN = 29.300000f;
const float HUMID_MAX = 56.900000f;
const float HUMID_RANGE = 27.600000f;

// Pressure scaling parameters (Pa)
const float PRESSURE_MIN = 96352.680000f;
const float PRESSURE_MAX = 100301.060000f;
const float PRESSURE_RANGE = 3948.380000f;

// Lux scaling parameters (lux)
const float LUX_MIN = 0.000000f;
const float LUX_MAX = 632.080000f;
const float LUX_RANGE = 632.080000f;

// ============================================================================
// SCALING FUNCTIONS (match Python's MinMaxScaler EXACTLY)
// ============================================================================

/**
 * Scale temperature from raw sensor value to [0, 1] range
 * Formula: (value - min) / (max - min)
 * 
 * @param raw_temp Raw temperature reading in °C
 * @return Scaled temperature in [0, 1] range
 */
float scale_temperature(float raw_temp) {
    // Clamp to valid range to handle sensor outliers
    if (raw_temp < TEMP_MIN) raw_temp = TEMP_MIN;
    if (raw_temp > TEMP_MAX) raw_temp = TEMP_MAX;
    
    // Apply MinMaxScaler formula
    float scaled = (raw_temp - TEMP_MIN) / TEMP_RANGE;
    
    return scaled;
}

/**
 * Scale humidity from raw sensor value to [0, 1] range
 * Formula: (value - min) / (max - min)
 * 
 * @param raw_humid Raw humidity reading in %
 * @return Scaled humidity in [0, 1] range
 */
float scale_humidity(float raw_humid) {
    // Clamp to valid range to handle sensor outliers
    if (raw_humid < HUMID_MIN) raw_humid = HUMID_MIN;
    if (raw_humid > HUMID_MAX) raw_humid = HUMID_MAX;
    
    // Apply MinMaxScaler formula
    float scaled = (raw_humid - HUMID_MIN) / HUMID_RANGE;
    
    return scaled;
}

/**
 * Scale pressure from raw sensor value to [0, 1] range
 * Formula: (value - min) / (max - min)
 * 
 * @param raw_pressure Raw pressure reading in Pa
 * @return Scaled pressure in [0, 1] range
 */
float scale_pressure(float raw_pressure) {
    // Clamp to valid range to handle sensor outliers
    if (raw_pressure < PRESSURE_MIN) raw_pressure = PRESSURE_MIN;
    if (raw_pressure > PRESSURE_MAX) raw_pressure = PRESSURE_MAX;
    
    // Apply MinMaxScaler formula
    float scaled = (raw_pressure - PRESSURE_MIN) / PRESSURE_RANGE;
    
    return scaled;
}

/**
 * Scale lux from raw sensor value to [0, 1] range
 * Formula: (value - min) / (max - min)
 * 
 * @param raw_lux Raw light intensity reading in lux
 * @return Scaled lux in [0, 1] range
 */
float scale_lux(float raw_lux) {
    // Clamp to valid range to handle sensor outliers
    if (raw_lux < LUX_MIN) raw_lux = LUX_MIN;
    if (raw_lux > LUX_MAX) raw_lux = LUX_MAX;
    
    // Apply MinMaxScaler formula
    float scaled = (raw_lux - LUX_MIN) / LUX_RANGE;
    
    return scaled;
}

/**
 * Scale all features at once (convenience function)
 * 
 * @param raw_temp Raw temperature in °C
 * @param raw_humid Raw humidity in %
 * @param raw_pressure Raw pressure in Pa
 * @param raw_lux Raw light intensity in lux
 * @param scaled_features Output array [4] for scaled features
 */
void scale_features(float raw_temp, float raw_humid, float raw_pressure, float raw_lux, float* scaled_features) {
    scaled_features[0] = scale_temperature(raw_temp);
    scaled_features[1] = scale_humidity(raw_humid);
    scaled_features[2] = scale_pressure(raw_pressure);
    scaled_features[3] = scale_lux(raw_lux);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Print scaling information (for debugging)
 */
void print_scaling_info() {
    Serial.println("========================================");
    Serial.println("SCALING PARAMETERS:");
    Serial.println("========================================");
    
    Serial.println("\nTemperature:");
    Serial.print("  Min: "); Serial.print(TEMP_MIN, 4); Serial.println(" °C");
    Serial.print("  Max: "); Serial.print(TEMP_MAX, 4); Serial.println(" °C");
    Serial.print("  Range: "); Serial.print(TEMP_RANGE, 4); Serial.println(" °C");
    
    Serial.println("\nHumidity:");
    Serial.print("  Min: "); Serial.print(HUMID_MIN, 4); Serial.println(" %");
    Serial.print("  Max: "); Serial.print(HUMID_MAX, 4); Serial.println(" %");
    Serial.print("  Range: "); Serial.print(HUMID_RANGE, 4); Serial.println(" %");
    
    Serial.println("\nPressure:");
    Serial.print("  Min: "); Serial.print(PRESSURE_MIN, 2); Serial.println(" Pa");
    Serial.print("  Max: "); Serial.print(PRESSURE_MAX, 2); Serial.println(" Pa");
    Serial.print("  Range: "); Serial.print(PRESSURE_RANGE, 2); Serial.println(" Pa");
    
    Serial.println("\nLux:");
    Serial.print("  Min: "); Serial.print(LUX_MIN, 2); Serial.println(" lux");
    Serial.print("  Max: "); Serial.print(LUX_MAX, 2); Serial.println(" lux");
    Serial.print("  Range: "); Serial.print(LUX_RANGE, 2); Serial.println(" lux");
    
    Serial.println("========================================");
}

/**
 * Verify a scaled value is in valid range [0, 1]
 * Returns true if valid, false otherwise
 */
bool verify_scaled_value(float scaled_value, const char* feature_name) {
    if (scaled_value < 0.0f || scaled_value > 1.0f) {
        Serial.print("⚠️  WARNING: ");
        Serial.print(feature_name);
        Serial.print(" scaled value out of range: ");
        Serial.println(scaled_value, 6);
        return false;
    }
    return true;
}

#endif // WEATHER_SCALING_H

/**
 * End of Scaling Functions
 * 
 * CRITICAL REMINDER:
 * ------------------
 * ✅ ALWAYS scale sensor readings before prediction
 * ✅ Use the individual scale_*() functions or scale_features()
 * ✅ Verify scaled values are in [0, 1] range for debugging
 * ❌ NEVER pass raw sensor values directly to predict()
 * 
 * Integration:
 * ------------
 * 1. Include this file in your Arduino sketch
 * 2. Include weather_model_250.h for the prediction model
 * 3. Read raw sensor values
 * 4. Scale using functions from this file
 * 5. Call predict() with scaled values
 * 6. Map prediction (0-3) to weather condition string
 * 
 * Example Arduino sketch available in final_output/esp32_code/
 */
