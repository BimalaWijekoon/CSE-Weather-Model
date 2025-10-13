// API Handler for ThingSpeak Integration
class ThingSpeakAPI {
    constructor() {
        this.baseUrl = CONFIG.apiUrl;
        this.channelId = CONFIG.channelId;
        this.cache = {
            lastFetch: null,
            data: null
        };
    }

    /**
     * Fetch latest channel feed data
     * @param {number} results - Number of results to fetch
     * @returns {Promise<Object>} Channel feed data
     */
    async getChannelFeed(results = 10) {
        try {
            // Build URL with Read API Key for private channel
            let url = `${this.baseUrl}/channels/${this.channelId}/feeds.json?results=${results}`;
            if (CONFIG.readApiKey) {
                url += `&api_key=${CONFIG.readApiKey}`;
            }
            
            const response = await fetch(url);
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            const data = await response.json();
            this.cache.lastFetch = new Date();
            this.cache.data = data;
            
            return data;
        } catch (error) {
            console.error('Error fetching channel feed:', error);
            throw error;
        }
    }

    /**
     * Fetch specific field data
     * @param {number} fieldNum - Field number (1-8)
     * @param {number} results - Number of results to fetch
     * @returns {Promise<Object>} Field data
     */
    async getFieldFeed(fieldNum, results = 100) {
        try {
            // Build URL with Read API Key for private channel
            let url = `${this.baseUrl}/channels/${this.channelId}/fields/${fieldNum}.json?results=${results}`;
            if (CONFIG.readApiKey) {
                url += `&api_key=${CONFIG.readApiKey}`;
            }
            
            const response = await fetch(url);
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            return await response.json();
        } catch (error) {
            console.error(`Error fetching field ${fieldNum}:`, error);
            throw error;
        }
    }

    /**
     * Get latest reading from channel
     * @returns {Promise<Object>} Latest sensor data
     */
    async getLatestData() {
        try {
            const data = await this.getChannelFeed(1);
            
            if (!data.feeds || data.feeds.length === 0) {
                throw new Error('No data available');
            }
            
            const latest = data.feeds[0];
            
            return {
                timestamp: new Date(latest.created_at),
                temperature: parseFloat(latest.field1) || 0,
                humidity: parseFloat(latest.field2) || 0,
                pressure: parseFloat(latest.field3) || 0,
                lux: parseFloat(latest.field4) || 0,
                gas: parseFloat(latest.field5) || 0,
                prediction: parseInt(latest.field6) || 0,
                inference: parseInt(latest.field7) || 0,
                rssi: parseInt(latest.field8) || 0
            };
        } catch (error) {
            console.error('Error getting latest data:', error);
            throw error;
        }
    }

    /**
     * Get historical data for graphs
     * @param {number} minutes - Time range in minutes
     * @returns {Promise<Array>} Array of historical data points
     */
    async getHistoricalData(minutes = 60) {
        try {
            const points = CONFIG.graphDataPoints[minutes.toString()] || 60;
            const data = await this.getChannelFeed(points);
            
            if (!data.feeds || data.feeds.length === 0) {
                return [];
            }
            
            return data.feeds.map(feed => ({
                timestamp: new Date(feed.created_at),
                temperature: parseFloat(feed.field1) || 0,
                humidity: parseFloat(feed.field2) || 0,
                pressure: parseFloat(feed.field3) || 0,
                lux: parseFloat(feed.field4) || 0,
                gas: parseFloat(feed.field5) || 0,
                prediction: parseInt(feed.field6) || 0,
                inference: parseInt(feed.field7) || 0,
                rssi: parseInt(feed.field8) || 0
            })).reverse(); // Reverse to get chronological order
        } catch (error) {
            console.error('Error getting historical data:', error);
            return [];
        }
    }

    /**
     * Get channel statistics
     * @returns {Promise<Object>} Channel statistics
     */
    async getChannelStats() {
        try {
            const data = await this.getChannelFeed(100);
            
            if (!data.feeds || data.feeds.length === 0) {
                return null;
            }
            
            const temps = data.feeds.map(f => parseFloat(f.field1)).filter(v => !isNaN(v));
            const humids = data.feeds.map(f => parseFloat(f.field2)).filter(v => !isNaN(v));
            const pressures = data.feeds.map(f => parseFloat(f.field3)).filter(v => !isNaN(v));
            
            return {
                totalRecords: data.channel.last_entry_id,
                averageTemp: this.calculateAverage(temps),
                averageHumid: this.calculateAverage(humids),
                averagePressure: this.calculateAverage(pressures),
                minTemp: Math.min(...temps),
                maxTemp: Math.max(...temps),
                minHumid: Math.min(...humids),
                maxHumid: Math.max(...humids)
            };
        } catch (error) {
            console.error('Error getting channel stats:', error);
            return null;
        }
    }

    /**
     * Calculate average of array
     * @param {Array<number>} values - Array of numbers
     * @returns {number} Average value
     */
    calculateAverage(values) {
        if (values.length === 0) return 0;
        return values.reduce((a, b) => a + b, 0) / values.length;
    }

    /**
     * Get gas quality level based on PPM
     * @param {number} ppm - Gas sensor reading in PPM
     * @returns {Object} Quality level info
     */
    getGasQuality(ppm) {
        const thresholds = CONFIG.gasThresholds;
        
        if (ppm < thresholds.good) {
            return { level: 'Good', color: '#10b981', icon: 'fa-check-circle' };
        } else if (ppm < thresholds.moderate) {
            return { level: 'Moderate', color: '#f59e0b', icon: 'fa-exclamation-triangle' };
        } else if (ppm < thresholds.unhealthy) {
            return { level: 'Unhealthy', color: '#ef4444', icon: 'fa-times-circle' };
        } else {
            return { level: 'Hazardous', color: '#991b1b', icon: 'fa-skull-crossbones' };
        }
    }

    /**
     * Get WiFi signal quality
     * @param {number} rssi - RSSI value in dBm
     * @returns {Object} Signal quality info
     */
    getWiFiQuality(rssi) {
        const quality = CONFIG.wifiQuality;
        
        if (rssi >= quality.excellent) {
            return { level: 'Excellent', bars: 5, color: '#10b981' };
        } else if (rssi >= quality.good) {
            return { level: 'Good', bars: 4, color: '#10b981' };
        } else if (rssi >= quality.fair) {
            return { level: 'Fair', bars: 3, color: '#f59e0b' };
        } else if (rssi >= quality.poor) {
            return { level: 'Poor', bars: 2, color: '#ef4444' };
        } else {
            return { level: 'Very Poor', bars: 1, color: '#ef4444' };
        }
    }

    /**
     * Get weather class info by ID
     * @param {number} classId - Weather class ID (0-4)
     * @returns {Object} Weather class info
     */
    getWeatherClass(classId) {
        return CONFIG.weatherClasses[classId] || CONFIG.weatherClasses[0];
    }

    /**
     * Format timestamp for display
     * @param {Date} date - Date object
     * @returns {string} Formatted time string
     */
    formatTime(date) {
        const now = new Date();
        const diff = now - date;
        const seconds = Math.floor(diff / 1000);
        const minutes = Math.floor(seconds / 60);
        const hours = Math.floor(minutes / 60);
        
        if (seconds < 60) {
            return `${seconds} seconds ago`;
        } else if (minutes < 60) {
            return `${minutes} minute${minutes > 1 ? 's' : ''} ago`;
        } else if (hours < 24) {
            return `${hours} hour${hours > 1 ? 's' : ''} ago`;
        } else {
            return date.toLocaleString();
        }
    }

    /**
     * Export data as CSV
     * @param {Array} data - Array of data points
     * @param {string} filename - Output filename
     */
    exportToCSV(data, filename = 'weather_data.csv') {
        if (!data || data.length === 0) {
            console.error('No data to export');
            return;
        }
        
        // CSV Headers
        const headers = [
            'Timestamp',
            'Temperature (°C)',
            'Humidity (%)',
            'Pressure (Pa)',
            'Light (lux)',
            'Gas (PPM)',
            'Prediction',
            'Inference (µs)',
            'WiFi RSSI (dBm)'
        ];
        
        // CSV Rows
        const rows = data.map(point => [
            point.timestamp.toISOString(),
            point.temperature.toFixed(2),
            point.humidity.toFixed(2),
            point.pressure.toFixed(2),
            point.lux.toFixed(2),
            point.gas.toFixed(2),
            this.getWeatherClass(point.prediction).name,
            point.inference,
            point.rssi
        ]);
        
        // Combine headers and rows
        const csv = [
            headers.join(','),
            ...rows.map(row => row.join(','))
        ].join('\n');
        
        // Create download link
        const blob = new Blob([csv], { type: 'text/csv' });
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();
        window.URL.revokeObjectURL(url);
    }

    /**
     * Check if cache is fresh
     * @param {number} maxAge - Maximum cache age in milliseconds
     * @returns {boolean} True if cache is fresh
     */
    isCacheFresh(maxAge = 10000) {
        if (!this.cache.lastFetch) return false;
        return (new Date() - this.cache.lastFetch) < maxAge;
    }
}

// Create global API instance
const api = new ThingSpeakAPI();

// OpenWeatherMap API Handler
class OpenWeatherAPI {
    constructor() {
        this.apiKey = CONFIG.openWeatherMap.apiKey;
        this.baseUrl = CONFIG.openWeatherMap.apiUrl;
        this.enabled = CONFIG.openWeatherMap.enabled && this.apiKey !== 'YOUR_API_KEY_HERE';
        this.cache = {
            lastFetch: null,
            data: null,
            location: null
        };
    }

    /**
     * Get user's geolocation
     * @returns {Promise<Object>} Coordinates {lat, lon}
     */
    async getGeolocation() {
        return new Promise((resolve, reject) => {
            if (!navigator.geolocation) {
                reject(new Error('Geolocation not supported'));
                return;
            }

            navigator.geolocation.getCurrentPosition(
                (position) => {
                    resolve({
                        lat: position.coords.latitude,
                        lon: position.coords.longitude
                    });
                },
                (error) => {
                    console.warn('Geolocation error:', error);
                    // Fallback to default location
                    resolve({
                        lat: CONFIG.geolocation.defaultLat,
                        lon: CONFIG.geolocation.defaultLon
                    });
                },
                { timeout: 5000 }
            );
        });
    }

    /**
     * Fetch current weather data
     * @param {number} lat - Latitude
     * @param {number} lon - Longitude
     * @returns {Promise<Object>} Weather data
     */
    async getCurrentWeather(lat, lon) {
        if (!this.enabled) {
            return this.getMockWeather();
        }

        try {
            const url = `${this.baseUrl}/weather?lat=${lat}&lon=${lon}&appid=${this.apiKey}&units=metric`;
            const response = await fetch(url);
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            const data = await response.json();
            this.cache.lastFetch = new Date();
            this.cache.data = data;
            
            return {
                location: data.name,
                country: data.sys.country,
                condition: data.weather[0].main,
                description: data.weather[0].description,
                temp: data.main.temp,
                feelsLike: data.main.feels_like,
                humidity: data.main.humidity,
                pressure: data.main.pressure,
                windSpeed: data.wind.speed,
                windDeg: data.wind.deg,
                clouds: data.clouds.all,
                icon: data.weather[0].icon,
                timestamp: new Date(data.dt * 1000)
            };
        } catch (error) {
            console.error('Error fetching weather:', error);
            return this.getMockWeather();
        }
    }

    /**
     * Get weather with auto-location
     * @returns {Promise<Object>} Weather data
     */
    async getWeatherAuto() {
        try {
            const coords = await this.getGeolocation();
            return await this.getCurrentWeather(coords.lat, coords.lon);
        } catch (error) {
            console.error('Error getting auto weather:', error);
            return this.getMockWeather();
        }
    }

    /**
     * Get mock weather data (fallback)
     * @returns {Object} Mock weather data
     */
    getMockWeather() {
        return {
            location: CONFIG.geolocation.defaultCity || 'Unknown',
            country: '--',
            condition: 'Clear',
            description: 'API key not configured',
            temp: 0,
            feelsLike: 0,
            humidity: 0,
            pressure: 0,
            windSpeed: 0,
            windDeg: 0,
            clouds: 0,
            icon: '01d',
            timestamp: new Date(),
            isMock: true
        };
    }

    /**
     * Map OpenWeatherMap condition to icon
     * @param {string} condition - Weather condition
     * @param {string} icon - OpenWeatherMap icon code
     * @returns {string} Font Awesome icon class
     */
    getWeatherIcon(condition, icon) {
        const isDay = icon.includes('d');
        
        const iconMap = {
            'Clear': isDay ? 'fa-sun' : 'fa-moon',
            'Clouds': 'fa-cloud',
            'Rain': 'fa-cloud-rain',
            'Drizzle': 'fa-cloud-rain',
            'Thunderstorm': 'fa-bolt',
            'Snow': 'fa-snowflake',
            'Mist': 'fa-smog',
            'Fog': 'fa-smog',
            'Haze': 'fa-smog',
            'Smoke': 'fa-smog'
        };
        
        return iconMap[condition] || 'fa-cloud';
    }

    /**
     * Check if cache is fresh
     * @returns {boolean} True if cache is fresh
     */
    isCacheFresh() {
        if (!this.cache.lastFetch) return false;
        const maxAge = CONFIG.openWeatherMap.updateInterval;
        return (new Date() - this.cache.lastFetch) < maxAge;
    }
}

// Create global weather API instance
const weatherAPI = new OpenWeatherAPI();
