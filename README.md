# Weather Prediction ML Project - ESP32-S3 Deployment

![Project Status](https://img.shields.io/badge/Status-Phase%201%20Complete-green)
![Python](https://img.shields.io/badge/Python-3.10+-blue)
![TensorFlow](https://img.shields.io/badge/TensorFlow-2.13+-orange)
![ESP32](https://img.shields.io/badge/Target-ESP32--S3-red)

## 📋 Project Overview

This project develops a machine learning model for weather prediction that will be deployed on an **ESP32-S3 microcontroller**. The system uses environmental sensors to predict weather conditions in real-time, making it suitable for IoT applications, smart agriculture, and environmental monitoring.

**Key Innovation:** Uses **YDF (Yggdrasil Decision Forests)** - Google's next-generation decision forest library for faster, more efficient model training and deployment.

**Course:** COE3012 Computer System Engineering  
**Institution:** [Your Institution]  
**Academic Year:** 2025

## 🎯 Objectives

- **Primary Goal:** Develop a YDF (Yggdrasil Decision Forests) model for weather classification
- **Target Platform:** ESP32-S3 microcontroller with optimized C++ deployment
- **Sensor Integration:** BME280, AHT10, and BH1750 environmental sensors
- **Real-time Prediction:** Classify weather conditions from sensor readings
- **Edge Deployment:** Optimize model for microcontroller constraints with microsecond inference

## 🔧 Hardware Components

### ESP32-S3 Microcontroller
- **CPU:** Dual-core Xtensa LX7, up to 240 MHz
- **Memory:** 512 KB SRAM, 384 KB ROM
- **Flash:** Up to 16 MB external flash
- **Connectivity:** Wi-Fi 802.11 b/g/n, Bluetooth 5.0
- **AI Acceleration:** Vector instructions for ML workloads

### Environmental Sensors

#### BME280 - Environmental Sensor
- **Temperature Range:** -40°C to +85°C (±1.0°C accuracy)
- **Humidity Range:** 0-100% RH (±3% accuracy)
- **Pressure Range:** 300-1100 hPa (±1 hPa accuracy)
- **Interface:** I2C/SPI
- **Use Case:** Primary environmental monitoring

#### AHT10 - Temperature & Humidity Sensor
- **Temperature Range:** -40°C to +85°C (±0.3°C accuracy)
- **Humidity Range:** 0-100% RH (±2% accuracy)
- **Interface:** I2C
- **Use Case:** Secondary temperature/humidity validation

#### BH1750 - Light Intensity Sensor
- **Light Range:** 1-65535 lux
- **Resolution:** 1 lux
- **Interface:** I2C
- **Use Case:** Ambient light measurement for weather context

## 🌦️ Weather Classification Categories

The model predicts the following weather conditions:

1. **Hot Humid** - High temperature (>25°C) + High humidity (>70%)
2. **Hot Dry** - High temperature (>25°C) + Low humidity (<40%)
3. **Cool** - Moderate temperature (10-25°C) + Moderate humidity (40-70%)
4. **Cold** - Low temperature (<10°C)
5. **Rainy** - High humidity (>80%) + Moderate light (<10,000 lux)
6. **Stormy** - High humidity (>85%) + Low light (<5,000 lux) + Variable pressure
7. **Sunny** - High light (>30,000 lux) + Low humidity (<50%)
8. **Cloudy** - Moderate light (5,000-30,000 lux) + Moderate humidity

## 🤖 Machine Learning Approach

### Algorithm: YDF (Yggdrasil Decision Forests)
**Google's next-generation decision forest library**

### **Advantages over TensorFlow Decision Forests:**
- ⚡ **No TensorFlow dependency conflicts** - Works with any Python version
- 🚀 **Faster training and inference** - Microsecond predictions vs milliseconds
- 🎯 **Cleaner, simpler API** - Less configuration complexity
- 📦 **Lighter weight** - ~500MB less memory usage than TF-DF
- 🔧 **Better compatibility** - Easier installation and setup
- 🎛️ **Advanced algorithms** - Latest research implementations

### **ESP32-S3 Deployment Strategy:**
1. **Training Phase:** YDF for fast, accurate model training
2. **Conversion Phase:** YDF model → Rule extraction → C++ code generation  
3. **Deployment Phase:** Lightweight decision rules on ESP32-S3
4. **Inference:** Direct rule-based implementation (no floating-point operations needed)

### Model Pipeline
1. **Data Collection:** Historical weather datasets + Synthetic sensor data
2. **Feature Engineering:** Sensor reading preprocessing and temporal features
3. **Model Training:** YDF Gradient Boosted Trees with automatic hyperparameter optimization
4. **Model Validation:** Cross-validation and performance metrics
5. **Model Conversion:** YDF model → C++ code for ESP32-S3
6. **Deployment:** Integration with ESP32-S3 firmware

### Performance Targets
- **Accuracy:** >85% on test set
- **Model Size:** <50KB for ESP32 deployment (much smaller than TF-Lite)
- **Inference Time:** <10ms per prediction (vs 100ms with TF-Lite)
- **Memory Usage:** <50KB RAM during inference (vs 200KB with TF-Lite)

## 📁 Project Structure

```
CSE Weather Model/
├── notebooks/                          # Jupyter notebooks for development
│   └── weather_prediction_model_training.ipynb
├── data/                               # Data storage
│   ├── raw/                           # Original datasets
│   ├── processed/                     # Cleaned and preprocessed data
│   └── external/                      # External datasets (Kaggle, APIs)
├── models/                            # Model artifacts
│   ├── trained/                       # Trained TensorFlow models
│   ├── converted/                     # TensorFlow Lite models
│   └── checkpoints/                   # Training checkpoints
├── src/                               # Source code modules
│   ├── data_processing/               # Data preprocessing scripts
│   ├── model_training/                # Training utilities
│   └── utils/                         # Helper functions
├── results/                           # Analysis outputs
│   ├── plots/                         # Visualizations
│   ├── reports/                       # Analysis reports
│   └── metrics/                       # Model performance metrics
├── deployment/                        # Deployment artifacts
│   └── esp32/                         # ESP32-specific code and models
├── requirements.txt                    # Python dependencies
├── README.md                          # This file
├── KAGGLE_SETUP.md                    # Kaggle API setup guide
└── HARDWARE_SPECS.md                  # Detailed hardware documentation
```

## 🚀 Quick Setup Instructions

### **Simplified Setup with YDF (Recommended)**

#### 1. Clone Repository
```bash
git clone [repository-url]
cd "CSE Weather Model"
```

#### 2. Create Virtual Environment
```bash
# Create virtual environment
python -m venv weather_ml_env

# Activate virtual environment
# Windows PowerShell:
.\weather_ml_env\Scripts\Activate.ps1

# Windows Command Prompt:
weather_ml_env\Scripts\activate.bat

# Linux/Mac:
source weather_ml_env/bin/activate
```

#### 3. Install YDF and Dependencies  
```bash
# Install YDF first (no TensorFlow conflicts!)
pip install ydf -U

# Install remaining requirements
pip install -r requirements.txt
```

#### 4. Verify Installation
```bash
python -c "import ydf; print(f'YDF Version: {ydf.__version__}')"
```

#### 5. Launch Jupyter Notebook
```bash
jupyter notebook notebooks/weather_prediction_model_training.ipynb
```

### **Benefits of YDF Setup:**
✅ **No version conflicts** - Works with any Python version  
✅ **Faster installation** - No heavy TensorFlow setup required  
✅ **Lighter footprint** - ~500MB less disk space  
✅ **Simpler workflow** - Less configuration needed

## 🔄 Development Workflow

### Phase 1: Project Setup & Environment ✅
- [x] Project structure creation
- [x] Virtual environment setup  
- [x] **YDF installation and configuration**
- [x] Dependency installation
- [x] Initial notebook creation
- [x] **YDF functionality verification**

### Phase 2: Data Collection & Preprocessing (Next)
- [ ] Download weather datasets
- [ ] Generate synthetic sensor data
- [ ] Data cleaning and validation
- [ ] Feature engineering optimized for YDF

### Phase 3: YDF Model Development
- [ ] **YDF Gradient Boosted Trees training**
- [ ] **YDF Random Forest comparison**
- [ ] Hyperparameter optimization with YDF
- [ ] Model validation and testing
- [ ] Performance analysis

### Phase 4: ESP32-S3 Optimization
- [ ] **YDF to C++ code conversion**
- [ ] **Rule-based inference implementation**
- [ ] ESP32 memory optimization
- [ ] Performance benchmarking
- [ ] Real-time inference testing

### Phase 5: Deployment
- [ ] ESP32-S3 firmware integration
- [ ] Hardware testing
- [ ] Real-world validation
- [ ] Documentation and presentation

## 🛠️ Dependencies

### Core ML Libraries
- `ydf>=0.5.0` - YDF (Yggdrasil Decision Forests) - Google's next-generation library
- `scikit-learn>=1.1.0` - Machine learning utilities
- `tensorflow>=2.13.0` - Optional: Only for final model conversion if needed

### Data Science
- `pandas>=1.5.0` - Data manipulation
- `numpy>=1.21.0` - Numerical computing
- `matplotlib>=3.5.0` - Plotting
- `seaborn>=0.11.0` - Statistical visualization
- `plotly>=5.0.0` - Interactive visualizations

### Development Tools
- `jupyter>=1.0.0` - Interactive development
- `kaggle>=1.5.0` - Dataset downloads
- `pytest>=7.0.0` - Testing framework

See `requirements.txt` for complete dependency list.

## 📊 Expected Datasets

### External Data Sources
1. **Historical Weather Data** (Kaggle)
   - Hourly weather measurements
   - Temperature, humidity, pressure, light conditions
   - Multi-year datasets for training

2. **Environmental Sensor Logs**
   - Real BME280, AHT10, BH1750 readings
   - Calibrated and validated measurements
   - Various weather conditions

3. **Synthetic Data Generation**
   - Simulate sensor readings within specifications
   - Generate edge cases and extreme conditions
   - Augment training dataset

## 🎯 Success Criteria

### Technical Metrics
- [ ] Model accuracy >85% on diverse test set
- [ ] TensorFlow Lite model size <500KB
- [ ] Inference time <100ms on ESP32-S3
- [ ] Memory usage <200KB during inference

### Project Deliverables
- [ ] Trained and validated ML model
- [ ] TensorFlow Lite model for ESP32-S3
- [ ] Complete documentation and analysis
- [ ] Working hardware prototype
- [ ] Academic presentation and report

## 👥 Team Collaboration

### Git Workflow
1. Create feature branches for development
2. Regular commits with descriptive messages
3. Pull requests for code review
4. Merge to main after validation

### Development Environment
- All team members use the same virtual environment
- Consistent Python version (3.10+)
- Standardized folder structure
- Shared configuration files

### Documentation Standards
- Clear code comments and docstrings
- Regular README updates
- Jupyter notebook documentation
- Hardware setup guides

## 🚨 Important Notes

### Academic Integrity
- This is an academic project for COE3012
- All work must be original and properly attributed
- External datasets and libraries must be cited
- Collaboration within team is encouraged

### Hardware Constraints
- ESP32-S3 has limited memory and processing power
- Model must be optimized for edge deployment
- Real-time performance requirements
- Power consumption considerations

### Safety and Testing
- Validate sensor readings against known references
- Test model performance in various environmental conditions
- Ensure robust error handling
- Document limitations and edge cases

## 📞 Support and Contact

### Team Members
- [Add team member names and roles]
- [Contact information]

### Course Information
- **Course:** COE3012 Computer System Engineering
- **Instructor:** [Instructor name]
- **Academic Year:** 2025

### Resources
- Course materials and lectures
- TensorFlow and ESP32 documentation
- Environmental sensor datasheets
- Academic papers on edge ML deployment

---

**Last Updated:** September 15, 2025  
**Project Phase:** 1 - Setup Complete ✅  
**Next Milestone:** Data Collection and Preprocessing
