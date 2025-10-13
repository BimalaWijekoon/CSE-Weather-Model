// Firebase API Handler for Historical Data
class FirebaseAPI {
    constructor() {
        this.db = null;
        this.auth = null;
        this.deviceId = null;
        this.initialized = false;
        this.authenticated = false;
        this.cache = {
            lastFetch: null,
            data: []
        };
    }

    /**
     * Initialize Firebase with configuration
     * @returns {Promise<boolean>} Success status
     */
    async initialize() {
        try {
            if (!CONFIG.firebase || !CONFIG.firebase.enabled) {
                console.warn('Firebase is not enabled in configuration');
                return false;
            }

            // Initialize Firebase
            if (!firebase.apps.length) {
                firebase.initializeApp(CONFIG.firebase.config);
            }

            this.db = firebase.database();
            this.auth = firebase.auth();
            
            // Authenticate with email/password
            if (CONFIG.firebase.auth && CONFIG.firebase.auth.email) {
                console.log('Authenticating with Firebase...');
                await this.auth.signInWithEmailAndPassword(
                    CONFIG.firebase.auth.email,
                    CONFIG.firebase.auth.password
                );
                this.authenticated = true;
                console.log('✅ Firebase authenticated successfully');
            } else {
                console.warn('No authentication credentials provided');
            }

            this.initialized = true;
            console.log('Firebase initialized successfully');
            return true;
        } catch (error) {
            console.error('Firebase initialization error:', error);
            this.initialized = false;
            this.authenticated = false;
            return false;
        }
    }

    /**
     * Get list of available device IDs
     * @returns {Promise<Array>} Array of device IDs
     */
    async getDevices() {
        try {
            if (!this.initialized) {
                await this.initialize();
            }

            const devicesRef = this.db.ref('devices');
            const snapshot = await devicesRef.once('value');
            
            if (!snapshot.exists()) {
                return [];
            }

            const devices = [];
            snapshot.forEach((child) => {
                const deviceData = child.val();
                const readings = deviceData.readings || {};
                
                // Get the most recent reading timestamp
                let lastTimestamp = null;
                if (Object.keys(readings).length > 0) {
                    const timestamps = Object.values(readings).map(r => r.timestamp).filter(t => t);
                    if (timestamps.length > 0) {
                        lastTimestamp = Math.max(...timestamps);
                    }
                }
                
                devices.push({
                    id: child.key,
                    info: deviceData.info || {},
                    lastSeen: lastTimestamp
                });
            });

            return devices;
        } catch (error) {
            console.error('Error fetching devices:', error);
            return [];
        }
    }

    /**
     * Set active device ID
     * @param {string} deviceId - Device MAC address
     */
    setDevice(deviceId) {
        this.deviceId = deviceId;
    }

    /**
     * Get device info
     * @param {string} deviceId - Device ID (optional, uses current device if not provided)
     * @returns {Promise<Object>} Device information
     */
    async getDeviceInfo(deviceId = null) {
        try {
            if (!this.initialized) {
                await this.initialize();
            }

            const id = deviceId || this.deviceId;
            if (!id) {
                throw new Error('No device ID specified');
            }

            const infoRef = this.db.ref(`devices/${id}/info`);
            const snapshot = await infoRef.once('value');
            
            return snapshot.val() || {};
        } catch (error) {
            console.error('Error fetching device info:', error);
            return {};
        }
    }

    /**
     * Get historical readings with date range filter
     * @param {Object} options - Query options
     * @param {Date} options.startDate - Start date
     * @param {Date} options.endDate - End date
     * @param {number} options.limit - Maximum number of results
     * @param {string} options.deviceId - Device ID (optional)
     * @returns {Promise<Array>} Array of readings
     */
    async getHistoricalData(options = {}) {
        try {
            if (!this.initialized) {
                await this.initialize();
            }

            const {
                startDate = new Date(Date.now() - 24 * 60 * 60 * 1000), // Default: last 24 hours
                endDate = new Date(),
                limit = 1000,
                deviceId = this.deviceId
            } = options;

            if (!deviceId) {
                throw new Error('No device ID specified');
            }

            const readingsRef = this.db.ref(`devices/${deviceId}/readings`);
            
            // Query with date range
            let query = readingsRef
                .orderByChild('timestamp')
                .startAt(startDate.getTime())
                .endAt(endDate.getTime());

            if (limit) {
                query = query.limitToLast(limit);
            }

            const snapshot = await query.once('value');
            
            if (!snapshot.exists()) {
                return [];
            }

            // Convert to array and sort by timestamp (descending)
            const readings = [];
            snapshot.forEach((child) => {
                const data = child.val();
                readings.push({
                    id: child.key,
                    timestamp: data.timestamp,
                    date: new Date(data.timestamp),
                    temperature: data.temperature,
                    humidity: data.humidity,
                    pressure: data.pressure,
                    lux: data.lux,
                    gas: data.gas || 0,
                    prediction: data.prediction,
                    predictionName: this.getPredictionName(data.prediction),
                    inferenceTime: data.inference_time
                });
            });

            // Sort by timestamp descending (newest first)
            readings.sort((a, b) => b.timestamp - a.timestamp);

            this.cache.lastFetch = new Date();
            this.cache.data = readings;

            return readings;
        } catch (error) {
            console.error('Error fetching historical data:', error);
            return [];
        }
    }

    /**
     * Get latest N readings
     * @param {number} count - Number of readings to fetch
     * @param {string} deviceId - Device ID (optional)
     * @returns {Promise<Array>} Array of readings
     */
    async getLatestReadings(count = 100, deviceId = null) {
        try {
            if (!this.initialized) {
                await this.initialize();
            }

            const id = deviceId || this.deviceId;
            if (!id) {
                throw new Error('No device ID specified');
            }

            const readingsRef = this.db.ref(`devices/${id}/readings`);
            const snapshot = await readingsRef
                .orderByChild('timestamp')
                .limitToLast(count)
                .once('value');

            if (!snapshot.exists()) {
                return [];
            }

            const readings = [];
            snapshot.forEach((child) => {
                const data = child.val();
                readings.push({
                    id: child.key,
                    timestamp: data.timestamp,
                    date: new Date(data.timestamp),
                    temperature: data.temperature,
                    humidity: data.humidity,
                    pressure: data.pressure,
                    lux: data.lux,
                    gas: data.gas || 0,
                    prediction: data.prediction,
                    predictionName: this.getPredictionName(data.prediction),
                    inferenceTime: data.inference_time
                });
            });

            // Sort by timestamp descending
            readings.sort((a, b) => b.timestamp - a.timestamp);

            return readings;
        } catch (error) {
            console.error('Error fetching latest readings:', error);
            return [];
        }
    }

    /**
     * Get statistics for a date range
     * @param {Array} readings - Array of readings
     * @returns {Object} Statistics object
     */
    calculateStatistics(readings) {
        if (!readings || readings.length === 0) {
            return {
                totalReadings: 0,
                dateRange: 'No data',
                avgTemperature: 0,
                avgHumidity: 0,
                avgPressure: 0,
                avgLux: 0,
                mostCommonWeather: 'N/A',
                weatherDistribution: {}
            };
        }

        // Calculate averages
        const avgTemp = readings.reduce((sum, r) => sum + r.temperature, 0) / readings.length;
        const avgHumid = readings.reduce((sum, r) => sum + r.humidity, 0) / readings.length;
        const avgPress = readings.reduce((sum, r) => sum + r.pressure, 0) / readings.length;
        const avgLux = readings.reduce((sum, r) => sum + r.lux, 0) / readings.length;

        // Weather distribution
        const weatherCount = {};
        readings.forEach(r => {
            const name = r.predictionName;
            weatherCount[name] = (weatherCount[name] || 0) + 1;
        });

        // Find most common weather
        let mostCommon = 'N/A';
        let maxCount = 0;
        Object.entries(weatherCount).forEach(([name, count]) => {
            if (count > maxCount) {
                maxCount = count;
                mostCommon = name;
            }
        });

        // Date range
        const oldest = readings[readings.length - 1].date;
        const newest = readings[0].date;
        const dateRange = `${this.formatDate(oldest)} - ${this.formatDate(newest)}`;

        return {
            totalReadings: readings.length,
            dateRange,
            avgTemperature: avgTemp.toFixed(1),
            avgHumidity: avgHumid.toFixed(1),
            avgPressure: avgPress.toFixed(2),
            avgLux: avgLux.toFixed(0),
            mostCommonWeather: mostCommon,
            weatherDistribution: weatherCount
        };
    }

    /**
     * Listen for real-time updates
     * @param {Function} callback - Callback function for new data
     * @param {string} deviceId - Device ID (optional)
     */
    listenForUpdates(callback, deviceId = null) {
        try {
            if (!this.initialized) {
                console.error('Firebase not initialized');
                return null;
            }

            const id = deviceId || this.deviceId;
            if (!id) {
                console.error('No device ID specified');
                return null;
            }

            const readingsRef = this.db.ref(`devices/${id}/readings`);
            
            // Listen for new child added
            const listener = readingsRef.limitToLast(1).on('child_added', (snapshot) => {
                const data = snapshot.val();
                const reading = {
                    id: snapshot.key,
                    timestamp: data.timestamp,
                    date: new Date(data.timestamp),
                    temperature: data.temperature,
                    humidity: data.humidity,
                    pressure: data.pressure,
                    lux: data.lux,
                    gas: data.gas || 0,
                    prediction: data.prediction,
                    predictionName: this.getPredictionName(data.prediction),
                    inferenceTime: data.inference_time
                };
                
                callback(reading);
            });

            return listener;
        } catch (error) {
            console.error('Error setting up real-time listener:', error);
            return null;
        }
    }

    /**
     * Stop listening for updates
     * @param {string} deviceId - Device ID (optional)
     */
    stopListening(deviceId = null) {
        try {
            const id = deviceId || this.deviceId;
            if (!id) return;

            const readingsRef = this.db.ref(`devices/${id}/readings`);
            readingsRef.off('child_added');
        } catch (error) {
            console.error('Error stopping listener:', error);
        }
    }

    /**
     * Export data to CSV
     * @param {Array} readings - Array of readings
     * @returns {string} CSV string
     */
    exportToCSV(readings) {
        if (!readings || readings.length === 0) {
            return '';
        }

        // CSV header
        const headers = [
            'Timestamp',
            'Date',
            'Temperature (°C)',
            'Humidity (%)',
            'Pressure (kPa)',
            'Light (lux)',
            'Gas (PPM)',
            'Weather Prediction',
            'Inference Time (ms)'
        ];

        // CSV rows
        const rows = readings.map(r => [
            r.timestamp,
            r.date.toLocaleString(),
            r.temperature,
            r.humidity,
            r.pressure,
            r.lux,
            r.gas,
            r.predictionName,
            r.inferenceTime
        ]);

        // Combine headers and rows
        const csvContent = [
            headers.join(','),
            ...rows.map(row => row.join(','))
        ].join('\n');

        return csvContent;
    }

    /**
     * Export data to JSON
     * @param {Array} readings - Array of readings
     * @returns {string} JSON string
     */
    exportToJSON(readings) {
        return JSON.stringify(readings, null, 2);
    }

    /**
     * Download data as file
     * @param {string} content - File content
     * @param {string} filename - File name
     * @param {string} mimeType - MIME type
     */
    downloadFile(content, filename, mimeType) {
        const blob = new Blob([content], { type: mimeType });
        const url = URL.createObjectURL(blob);
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);
    }

    /**
     * Get weather prediction name from class ID
     * @param {number} classId - Prediction class ID
     * @returns {string} Weather name
     */
    getPredictionName(classId) {
        const weather = CONFIG.weatherClasses.find(w => w.id === classId);
        return weather ? weather.name : 'Unknown';
    }

    /**
     * Get weather prediction color
     * @param {number} classId - Prediction class ID
     * @returns {string} Color hex code
     */
    getPredictionColor(classId) {
        const weather = CONFIG.weatherClasses.find(w => w.id === classId);
        return weather ? weather.color : '#666666';
    }

    /**
     * Format date for display
     * @param {Date} date - Date object
     * @returns {string} Formatted date string
     */
    formatDate(date) {
        return date.toLocaleDateString('en-US', {
            year: 'numeric',
            month: 'short',
            day: 'numeric'
        });
    }

    /**
     * Format datetime for display
     * @param {Date} date - Date object
     * @returns {string} Formatted datetime string
     */
    formatDateTime(date) {
        return date.toLocaleString('en-US', {
            year: 'numeric',
            month: 'short',
            day: 'numeric',
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit'
        });
    }

    /**
     * Get all readings for a device with optional limit
     * @param {string} deviceId - Device ID
     * @param {number} limit - Maximum number of readings (optional)
     * @returns {Promise<Array>} Array of readings
     */
    async getAllReadings(deviceId = null, limit = null) {
        try {
            if (!this.initialized) {
                await this.initialize();
            }

            const id = deviceId || this.deviceId;
            if (!id) {
                throw new Error('No device ID specified');
            }

            const readingsRef = this.db.ref(`devices/${id}/readings`);
            
            let query = readingsRef.orderByKey();
            if (limit) {
                query = query.limitToLast(limit);
            }

            const snapshot = await query.once('value');
            
            if (!snapshot.exists()) {
                return [];
            }

            const readings = [];
            snapshot.forEach((child) => {
                readings.push({
                    timestamp: parseInt(child.key),
                    ...child.val()
                });
            });

            // Sort by timestamp descending (newest first)
            readings.sort((a, b) => b.timestamp - a.timestamp);
            
            return readings;
        } catch (error) {
            console.error('Error fetching all readings:', error);
            return [];
        }
    }

    /**
     * Get readings statistics
     * @param {string} deviceId - Device ID
     * @returns {Promise<Object>} Statistics object
     */
    async getReadingsStats(deviceId = null) {
        try {
            const readings = await this.getAllReadings(deviceId);
            
            if (readings.length === 0) {
                return {
                    total: 0,
                    today: 0,
                    avgPerHour: 0,
                    weatherDistribution: {}
                };
            }

            // Count today's readings
            const now = Date.now();
            const todayStart = new Date();
            todayStart.setHours(0, 0, 0, 0);
            const todayCount = readings.filter(r => (r.timestamp * 1000) >= todayStart.getTime()).length;

            // Calculate average per hour
            const oldestReading = readings[readings.length - 1];
            const hoursElapsed = (now - (oldestReading.timestamp * 1000)) / (1000 * 60 * 60);
            const avgPerHour = hoursElapsed > 0 ? (readings.length / hoursElapsed).toFixed(1) : 0;

            // Weather distribution
            const weatherDist = {};
            readings.forEach(r => {
                const weather = r.prediction || 'Unknown';
                weatherDist[weather] = (weatherDist[weather] || 0) + 1;
            });

            return {
                total: readings.length,
                today: todayCount,
                avgPerHour: avgPerHour,
                weatherDistribution: weatherDist
            };
        } catch (error) {
            console.error('Error calculating statistics:', error);
            return {
                total: 0,
                today: 0,
                avgPerHour: 0,
                weatherDistribution: {}
            };
        }
    }

    /**
     * Get device status
     * @param {string} deviceId - Device ID
     * @returns {Promise<Object>} Status object
     */
    async getDeviceStatus(deviceId = null) {
        try {
            if (!this.initialized) {
                await this.initialize();
            }

            const id = deviceId || this.deviceId;
            if (!id) {
                throw new Error('No device ID specified');
            }

            const statusRef = this.db.ref(`devices/${id}/status`);
            const snapshot = await statusRef.once('value');
            
            if (!snapshot.exists()) {
                return { online: false, last_seen: null };
            }

            return snapshot.val();
        } catch (error) {
            console.error('Error fetching device status:', error);
            return { online: false, last_seen: null };
        }
    }

    /**
     * Filter readings by weather type
     * @param {Array} readings - Array of readings
     * @param {string} weatherType - Weather type to filter
     * @returns {Array} Filtered readings
     */
    filterByWeather(readings, weatherType) {
        if (!weatherType || weatherType === 'all') {
            return readings;
        }
        return readings.filter(r => r.prediction === weatherType);
    }

    /**
     * Export readings to CSV format
     * @param {Array} readings - Array of readings
     * @returns {string} CSV string
     */
    exportToCSV(readings) {
        if (!readings || readings.length === 0) {
            return '';
        }

        const headers = ['Timestamp', 'Date/Time', 'Temperature', 'Humidity', 'Pressure', 'Lux', 'Prediction', 'Inference Time'];
        const rows = readings.map(r => {
            const date = new Date(r.timestamp * 1000);
            return [
                r.timestamp,
                this.formatDateTime(date),
                r.temperature?.toFixed(2) || '',
                r.humidity?.toFixed(2) || '',
                r.pressure?.toFixed(2) || '',
                r.lux?.toFixed(2) || '',
                r.prediction || '',
                r.inference_time || ''
            ];
        });

        const csvContent = [
            headers.join(','),
            ...rows.map(row => row.join(','))
        ].join('\n');

        return csvContent;
    }

    /**
     * Download CSV file
     * @param {Array} readings - Array of readings
     * @param {string} filename - Filename for download
     */
    downloadCSV(readings, filename = 'firebase_backup_data.csv') {
        const csv = this.exportToCSV(readings);
        const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
        const link = document.createElement('a');
        const url = URL.createObjectURL(blob);
        
        link.setAttribute('href', url);
        link.setAttribute('download', filename);
        link.style.visibility = 'hidden';
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);
    }

    /**
     * Download JSON file
     * @param {Array} readings - Array of readings
     * @param {string} filename - Filename for download
     */
    downloadJSON(readings, filename = 'firebase_backup_data.json') {
        const jsonStr = JSON.stringify(readings, null, 2);
        const blob = new Blob([jsonStr], { type: 'application/json' });
        const link = document.createElement('a');
        const url = URL.createObjectURL(blob);
        
        link.setAttribute('href', url);
        link.setAttribute('download', filename);
        link.style.visibility = 'hidden';
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);
    }
}

// Create global instance
const firebaseAPI = new FirebaseAPI();
