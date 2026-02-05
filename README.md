# 🌦️ ESP32-S3 Weather Prediction System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python](https://img.shields.io/badge/Python-3.8%2B-blue.svg)](https://www.python.org/)
[![ESP32](https://img.shields.io/badge/ESP32-S3-green.svg)](https://www.espressif.com/en/products/socs/esp32-s3)

An intelligent IoT-based weather monitoring and prediction system that combines real-time environmental sensing with machine learning to provide accurate weather forecasts. This project deploys a trained Random Forest classifier on an ESP32-S3 microcontroller for edge computing weather predictions.

## 📋 Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [System Architecture](#system-architecture)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

## 🎯 Overview

The **ESP32-S3 Weather Prediction System** is designed for the COE3012 Computer System Engineering course. It demonstrates the practical deployment of machine learning on resource-constrained microcontrollers, capable of predicting weather conditions (Cloudy, Foggy, Rainy, Stormy, Sunny) based on multiple sensor inputs.

### System Objectives

- 🔴 **Real-time Monitoring:** Collect environmental data from multiple sensors every second
- 🧠 **Intelligent Prediction:** Use machine learning to predict weather conditions
- ⚡ **Edge Computing:** Run ML inference directly on ESP32-S3 without cloud dependency
- ☁️ **Cloud Integration:** Upload sensor data and predictions to ThingSpeak and Firebase
- 📊 **Data Visualization:** Provide a responsive web dashboard for real-time monitoring

## ✨ Key Features

### Hardware Features
- **ESP32-S3** microcontroller with dual-core processor
- **2.5 MB** PSRAM for model storage
- **Multi-sensor integration** (Temperature, Humidity, Pressure, Light, Air Quality)
- **RGB LED** for visual status indication
- **WiFi connectivity** for cloud integration

### Software Features
- **Modular code architecture** with separate header files
- **100% prediction accuracy** on test data
- **Fast inference** (~2-3 milliseconds per prediction)
- **Real-time cloud synchronization** with ThingSpeak and Firebase
- **Interactive web dashboard** with Chart.js visualizations
- **Automatic WiFi reconnection** and error handling

### Machine Learning
- **Random Forest Classifier** with 250 decision trees
- **YDF (Yggdrasil Decision Forests)** for model training
- **C++ model conversion** for embedded deployment
- **MinMaxScaler normalization** (0-1 scaling)
- **5-class classification:** Cloudy, Foggy, Rainy, Stormy, Sunny

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32-S3 MICROCONTROLLER                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  SENSOR LAYER                                            │  │
│  │  • Temperature & Humidity (DHT22/AHT10)                  │  │
│  │  • Pressure (BMP280/BME280)                              │  │
│  │  • Light Intensity (BH1750)                              │  │
│  │  • Air Quality (MQ-135)                                  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                           ↓                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  DATA PROCESSING LAYER                                   │  │
│  │  • MinMaxScaler (0-1 normalization)                      │  │
│  │  • Feature averaging (5-second buffer)                   │  │
│  └──────────────────────────────────────────────────────────┘  │
│                           ↓                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  MACHINE LEARNING LAYER                                  │  │
│  │  • RandomForest Classifier (250 trees)                   │  │
│  │  • C++ Embedded Model                                    │  │
│  │  • Inference Time: ~2000-3000 µs                         │  │
│  └──────────────────────────────────────────────────────────┘  │
│                           ↓                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  CLOUD COMMUNICATION LAYER                               │  │
│  │  • WiFi Manager (Auto-reconnect)                         │  │
│  │  • ThingSpeak (Real-time upload)                         │  │
│  │  • Firebase (Data backup)                                │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                    CLOUD PLATFORMS                              │
│  • ThingSpeak Channel ID: 3108323                               │
│  • Firebase Realtime Database                                   │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                  WEB DASHBOARD (Frontend)                       │
│  • Real-time sensor monitoring                                  │
│  • Live prediction display                                      │
│  • Historical data graphs (Chart.js)                            │
│  • Device status monitoring                                     │
└─────────────────────────────────────────────────────────────────┘
```

## 🔧 Hardware Requirements

| Component | Model | Purpose |
|-----------|-------|---------|
| Microcontroller | ESP32-S3 | Main processing unit with WiFi |
| Temperature/Humidity | DHT22 or AHT10 | Environmental sensing |
| Pressure Sensor | BMP280 or BME280 | Atmospheric pressure measurement |
| Light Sensor | BH1750 | Ambient light intensity |
| Gas Sensor | MQ-135 | Air quality monitoring |
| RGB LED | WS2812B or similar | Visual status indication |

### Wiring Information
See `WIRING_DIAGRAM_FIXED.txt` for detailed connection diagrams.

## 💻 Software Requirements

### Python Environment
```bash
Python 3.8 or higher
```

### Python Dependencies
Install all required packages using:
```bash
pip install -r requirements.txt
```

Key packages include:
- **ydf** (Yggdrasil Decision Forests) for ML training
- **pandas**, **numpy**, **scikit-learn** for data processing
- **matplotlib**, **seaborn**, **plotly** for visualization
- **jupyter** for notebook development

### ESP32 Development
- **Arduino IDE** 1.8.x or 2.x
- **ESP32 Board Support** (Espressif Systems)
- Required Arduino Libraries:
  - WiFi
  - HTTPClient
  - ArduinoJson
  - Adafruit Unified Sensor
  - DHT sensor library
  - BMP280/BME280 library
  - BH1750 library

## 📥 Installation

### 1. Clone the Repository
```bash
git clone https://github.com/BimalaWijekoon/CSE-Weather-Model.git
cd CSE-Weather-Model
```

### 2. Install Python Dependencies
```bash
pip install -r requirements.txt
```

### 3. Train the Model (Optional)
If you want to retrain the model:
```bash
jupyter notebook notebooks/FINAL_Weather_Model_Training.ipynb
```

### 4. Deploy to ESP32
1. Open Arduino IDE
2. Load the ESP32 code from `final_output/esp32_code/`
3. Configure WiFi credentials and API keys
4. Upload to ESP32-S3 board

### 5. Setup Frontend Dashboard
```bash
cd frontend
# Open index.html in a web browser
# Or run the START_DASHBOARD.bat for Windows
```

## 🚀 Usage

### Running the System

1. **Power on the ESP32-S3** with all sensors connected
2. The device will:
   - Connect to WiFi
   - Start reading sensors every second
   - Make weather predictions using the embedded ML model
   - Upload data to ThingSpeak and Firebase every 15 seconds
   - Display status via RGB LED

3. **Access the Dashboard**
   - Open `frontend/index.html` in a web browser
   - View real-time sensor data and predictions
   - Monitor historical trends and device status

### LED Status Indicators
- 🔵 **Blue:** WiFi connecting
- 🟢 **Green:** System operating normally
- 🟡 **Yellow:** Uploading data to cloud
- 🔴 **Red:** Error detected
- 🟣 **Purple:** Making prediction

## 📁 Project Structure

```
CSE-Weather-Model/
├── data/                          # Training datasets
│   └── raw/                       # Raw weather data
├── notebooks/                     # Jupyter notebooks
│   └── FINAL_Weather_Model_Training.ipynb
├── final_output/                  # Trained models and deployment code
│   ├── models/                    # Trained ML models
│   ├── esp32_code/                # ESP32 Arduino code
│   └── metrics/                   # Model performance metrics
├── frontend/                      # Web dashboard
│   ├── css/                       # Stylesheets
│   ├── js/                        # JavaScript files
│   └── START_DASHBOARD.bat        # Dashboard launcher
├── .firebase/                     # Firebase configuration
├── .firebaserc                    # Firebase project config
├── requirements.txt               # Python dependencies
├── PROJECT_DOCUMENTATION.md       # Complete technical documentation
├── ESP32_SIMULATION_UPDATED.md    # ESP32 simulation guide
├── ESP32_BEFORE_AFTER_COMPARISON.md  # Model comparison
├── FIREBASE_HOSTING_SETUP.md      # Firebase setup guide
├── WIRING_DIAGRAM_FIXED.txt       # Hardware wiring diagram
└── README.md                      # This file
```

## 📚 Documentation

For detailed technical information, please refer to:

- **[PROJECT_DOCUMENTATION.md](PROJECT_DOCUMENTATION.md)** - Complete system documentation including:
  - Dataset details and data quality analysis
  - Machine learning model training process
  - Model conversion and deployment steps
  - ESP32 code architecture
  - Cloud integration setup
  - Frontend dashboard details

- **[ESP32_SIMULATION_UPDATED.md](ESP32_SIMULATION_UPDATED.md)** - ESP32 simulation code and testing

- **[ESP32_BEFORE_AFTER_COMPARISON.md](ESP32_BEFORE_AFTER_COMPARISON.md)** - Model performance comparison

- **[FIREBASE_HOSTING_SETUP.md](FIREBASE_HOSTING_SETUP.md)** - Firebase configuration guide

## 🎓 Course Information

**Course:** COE3012 Computer System Engineering  
**Date:** October 2025  
**Institution:** [Your Institution Name]

## 👥 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- **YDF (Yggdrasil Decision Forests)** for the machine learning framework
- **ThingSpeak** for IoT data platform
- **Firebase** for real-time database
- **Espressif Systems** for ESP32-S3 platform
- **OpenWeatherMap** for weather comparison API

---

**Built with ❤️ for Computer System Engineering**
