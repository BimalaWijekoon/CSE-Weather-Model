// Main Application Controller
class WeatherDashboard {
    constructor() {
        this.currentPage = 'dashboard';
        this.autoRefresh = true;
        this.refreshIntervals = {};
        this.timeRange = 60; // Default 1 hour
        this.darkMode = false;
        this.activityLog = [];
        this.autoScroll = true;
        this.lastDataHash = null;
        this.lastPrediction = null; // Track previous prediction
        
        this.init();
    }

    /**
     * Initialize the application
     */
    async init() {
        // Setup event listeners
        this.setupNavigation();
        this.setupControls();
        this.setupTheme();
        
        // Initialize charts
        chartManager.initializeCharts();
        
        // Load initial data
        await this.loadInitialData();
        
        // Start auto-refresh
        this.startAutoRefresh();
        
        console.log('Weather Dashboard initialized successfully');
    }

    /**
     * Setup navigation event listeners
     */
    setupNavigation() {
        // Sidebar navigation
        document.querySelectorAll('.nav-item').forEach(item => {
            item.addEventListener('click', (e) => {
                e.preventDefault();
                const page = item.dataset.page;
                this.navigateToPage(page);
            });
        });

        // Mobile toggle
        const toggleBtn = document.getElementById('toggleSidebar');
        const sidebar = document.getElementById('sidebar');
        
        if (toggleBtn) {
            toggleBtn.addEventListener('click', () => {
                sidebar.classList.toggle('open');
            });
        }

        // Close sidebar on mobile when clicking outside
        document.addEventListener('click', (e) => {
            if (window.innerWidth <= 768) {
                if (!sidebar.contains(e.target) && sidebar.classList.contains('open')) {
                    sidebar.classList.remove('open');
                }
            }
        });
    }

    /**
     * Setup control event listeners
     */
    setupControls() {
        // Refresh button
        const refreshBtn = document.getElementById('refreshBtn');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => {
                this.refreshCurrentPage();
            });
        }

        // Time range selector
        document.querySelectorAll('.time-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                document.querySelectorAll('.time-btn').forEach(b => b.classList.remove('active'));
                e.target.classList.add('active');
                this.timeRange = parseInt(e.target.dataset.range);
                this.updateGraphs();
            });
        });

        // Auto-refresh toggle
        const autoRefreshToggle = document.getElementById('autoRefreshToggle');
        if (autoRefreshToggle) {
            autoRefreshToggle.addEventListener('change', (e) => {
                this.autoRefresh = e.target.checked;
                if (this.autoRefresh) {
                    this.startAutoRefresh();
                } else {
                    this.stopAutoRefresh();
                }
            });
        }

        // Export data button
        const exportBtn = document.getElementById('exportDataBtn');
        if (exportBtn) {
            exportBtn.addEventListener('click', () => {
                this.exportData();
            });
        }
        
        // Clear log button
        const clearLogBtn = document.getElementById('clearLogBtn');
        if (clearLogBtn) {
            clearLogBtn.addEventListener('click', () => {
                this.clearActivityLog();
            });
        }
        
        // Auto-scroll toggle
        const autoScrollToggle = document.getElementById('autoScrollToggle');
        if (autoScrollToggle) {
            autoScrollToggle.addEventListener('change', (e) => {
                this.autoScroll = e.target.checked;
            });
        }
    }

    /**
     * Setup theme toggle
     */
    setupTheme() {
        const themeToggle = document.getElementById('themeToggle');
        const html = document.documentElement;
        
        // Load saved theme
        const savedTheme = localStorage.getItem('theme') || 'light';
        this.darkMode = savedTheme === 'dark';
        html.setAttribute('data-theme', savedTheme);
        this.updateThemeButton();
        
        if (themeToggle) {
            themeToggle.addEventListener('click', () => {
                this.darkMode = !this.darkMode;
                const theme = this.darkMode ? 'dark' : 'light';
                html.setAttribute('data-theme', theme);
                localStorage.setItem('theme', theme);
                this.updateThemeButton();
                chartManager.updateTheme();
            });
        }
    }

    /**
     * Update theme button text
     */
    updateThemeButton() {
        const btn = document.getElementById('themeToggle');
        if (!btn) return;
        
        const icon = btn.querySelector('i');
        const span = btn.querySelector('span');
        
        if (this.darkMode) {
            icon.className = 'fas fa-sun';
            span.textContent = 'Light Mode';
        } else {
            icon.className = 'fas fa-moon';
            span.textContent = 'Dark Mode';
        }
    }

    /**
     * Navigate to a page
     * @param {string} pageName - Page name
     */
    navigateToPage(pageName) {
        // Update active nav item
        document.querySelectorAll('.nav-item').forEach(item => {
            item.classList.remove('active');
            if (item.dataset.page === pageName) {
                item.classList.add('active');
            }
        });

        // Update active page
        document.querySelectorAll('.page').forEach(page => {
            page.classList.remove('active');
        });
        
        const targetPage = document.getElementById(`page-${pageName}`);
        if (targetPage) {
            targetPage.classList.add('active');
        }

        // Update page title
        const titles = {
            'dashboard': 'Dashboard',
            'graphs': 'Real-Time Graphs',
            'predictions': 'ML Predictions',
            'activity': 'Activity Log',
            'backup': 'Backup & Analysis',
            'device': 'Device Status',
            'wifi': 'WiFi Overview'
        };
        
        document.getElementById('pageTitle').textContent = titles[pageName] || 'Dashboard';
        this.currentPage = pageName;

        // Load page-specific data
        this.loadPageData(pageName);
    }

    /**
     * Load initial data
     */
    async loadInitialData() {
        try {
            this.setConnectionStatus('connecting');
            
            // Test connection
            await api.getLatestData();
            
            this.setConnectionStatus('connected');
            
            // Load dashboard data
            await this.updateDashboard();
            
            // Load real weather
            await this.updateRealWeather();
            
        } catch (error) {
            console.error('Failed to load initial data:', error);
            this.setConnectionStatus('disconnected');
        }
    }

    /**
     * Load page-specific data
     * @param {string} pageName - Page name
     */
    async loadPageData(pageName) {
        switch (pageName) {
            case 'dashboard':
                await this.updateDashboard();
                break;
            case 'graphs':
                await this.updateGraphs();
                break;
            case 'predictions':
                await this.updatePredictions();
                break;
            case 'activity':
                await this.updateActivityLog();
                break;
            case 'backup':
                await this.updateBackupPage();
                break;
            case 'device':
                await this.updateDeviceStatus();
                break;
            case 'wifi':
                await this.updateWiFiStatus();
                break;
        }
    }

    /**
     * Update dashboard with latest data
     */
    async updateDashboard() {
        try {
            const latest = await api.getLatestData();
            const stats = await api.getChannelStats();
            
            // Check if data actually changed
            const dataHash = JSON.stringify(latest);
            const dataChanged = dataHash !== this.lastDataHash;
            this.lastDataHash = dataHash;
            
            // Update sensor values with flash animation
            this.updateValueWithFlash('tempValue', latest.temperature.toFixed(2), dataChanged);
            this.updateValueWithFlash('humidValue', latest.humidity.toFixed(2), dataChanged);
            this.updateValueWithFlash('pressureValue', latest.pressure.toFixed(0), dataChanged);
            this.updateValueWithFlash('luxValue', latest.lux.toFixed(2), dataChanged);
            this.updateValueWithFlash('gasValue', latest.gas.toFixed(1), dataChanged);
            
            // Pulse cards on update
            if (dataChanged) {
                this.pulseCards();
            }
            
            // Update trends (placeholder - would need historical comparison)
            this.updateTrend('tempTrend', 'up');
            this.updateTrend('humidTrend', 'down');
            this.updateTrend('pressureTrend', 'up');
            this.updateTrend('luxTrend', 'up');
            this.updateTrend('gasTrend', 'down');
            
            // Update prediction
            const weather = api.getWeatherClass(latest.prediction);
            this.updateElement('predictionValue', weather.name);
            this.updatePredictionIcon(weather);
            this.updateElement('predictionConfidence', '95%'); // Placeholder
            this.updateElement('inferenceTime', latest.inference.toLocaleString());
            
            // Update status row
            this.updateElement('wifiSignal', `${latest.rssi} dBm`);
            this.updateElement('uploadStatus', '✅ Active');
            if (stats) {
                this.updateElement('totalRecords', stats.totalRecords.toLocaleString());
            }
            
            // Update mini status cards in top bar
            this.updateElement('miniWifiSignal', `${latest.rssi} dBm`);
            this.updateElement('miniUploadStatus', 'Active');
            if (stats) {
                this.updateElement('miniTotalRecords', stats.totalRecords.toLocaleString());
            }
            this.updateElement('miniInferenceTime', latest.inference.toLocaleString());
            
            // Update ML prediction card in weather comparison section
            const predictedWeather = api.getWeatherClass(latest.prediction);
            this.updateElement('mlPredictionValue', predictedWeather.name);
            this.updateElement('mlPredictionTemp', `${latest.temperature.toFixed(1)} °C`);
            this.updateElement('mlPredictionHumidity', `${latest.humidity.toFixed(0)} %`);
            this.updateElement('mlPredictionPressure', `${(latest.pressure / 1000).toFixed(1)} kPa`);
            
            // Update ML prediction icon
            const mlIconElement = document.getElementById('mlWeatherIcon');
            if (mlIconElement) {
                const weatherIconClass = this.getWeatherIconClass(predictedWeather.name);
                mlIconElement.className = `weather-icon-large ${weatherIconClass}`;
            }
            
            // Update sparklines
            chartManager.updateAllSparklines(latest);
            
            // Update last update time
            this.updateLastUpdateTime(latest.timestamp);
            
            // Update live indicator
            this.setLiveIndicator(true);
            
            // Log activity
            this.addLogEntry('success', 'Data updated successfully', {
                'Temperature': `${latest.temperature.toFixed(2)} °C`,
                'Humidity': `${latest.humidity.toFixed(2)} %`,
                'Prediction': weather.name,
                'RSSI': `${latest.rssi} dBm`
            });
            
        } catch (error) {
            console.error('Error updating dashboard:', error);
            this.setConnectionStatus('disconnected');
            this.setLiveIndicator(false);
            this.addLogEntry('error', `Failed to update: ${error.message}`);
        }
    }

    /**
     * Update real-time graphs
     */
    async updateGraphs() {
        try {
            const data = await api.getHistoricalData(this.timeRange);
            
            if (data.length > 0) {
                // Update charts
                chartManager.updateAllGraphs(data);
                
                // Update current values in graph headers
                const latest = data[data.length - 1];
                this.updateElement('graphTemp', `${latest.temperature.toFixed(2)} °C`);
                this.updateElement('graphHumid', `${latest.humidity.toFixed(2)} %`);
                this.updateElement('graphPressure', `${latest.pressure.toFixed(0)} Pa`);
                this.updateElement('graphLux', `${latest.lux.toFixed(2)} lux`);
                this.updateElement('graphGas', `${latest.gas.toFixed(1)} PPM`);
            }
            
        } catch (error) {
            console.error('Error updating graphs:', error);
        }
    }

    /**
     * Update real weather data from OpenWeatherMap API
     */
    async updateRealWeather() {
        try {
            if (!CONFIG.openWeatherMap.enabled) {
                console.log('OpenWeatherMap API disabled - showing placeholder');
                this.updateElement('realWeatherLocation', 'API Not Configured');
                this.updateElement('realWeatherCondition', 'Add API key in config.js');
                this.updateElement('realWeatherTemp', '-- °C');
                this.updateElement('realWeatherHumidity', '-- %');
                this.updateElement('realWeatherWind', '-- km/h');
                return;
            }

            const weatherData = await weatherAPI.getWeatherAuto();
            
            if (!weatherData) {
                throw new Error('No weather data received');
            }
            
            // Update location in header
            const locationElement = document.querySelector('#weatherLocation span');
            if (locationElement) {
                locationElement.textContent = weatherData.location || 'Unknown';
            }
            
            // Update icon with proper Font Awesome icon
            const iconElement = document.getElementById('realWeatherIcon');
            if (iconElement) {
                iconElement.innerHTML = `<i class="fas ${weatherData.icon || 'fa-cloud'}"></i>`;
            }
            
            // Update condition
            this.updateElement('realWeatherCondition', weatherData.condition || 'N/A');
            
            // Update weather details - use temperature or temp
            const temp = weatherData.temperature || weatherData.temp || 0;
            this.updateElement('realWeatherTemp', `${temp.toFixed(1)} °C`);
            this.updateElement('realWeatherHumidity', `${weatherData.humidity || 0} %`);
            this.updateElement('realWeatherWind', `${(weatherData.windSpeed || 0).toFixed(1)} km/h`);
            
            // Log success only if not mock data
            if (!weatherData.isMock) {
                this.addLogEntry('success', 'Real weather updated', {
                    'Location': weatherData.location,
                    'Condition': weatherData.condition,
                    'Temperature': `${temp.toFixed(1)} °C`
                });
            }
            
        } catch (error) {
            console.error('Error updating real weather:', error);
            this.updateElement('realWeatherLocation', 'Weather Unavailable');
            this.updateElement('realWeatherCondition', 'Failed to load');
            this.addLogEntry('warning', `Weather API error: ${error.message}`);
        }
    }

    /**
     * Get weather icon class based on condition name
     * @param {string} weatherName - Weather condition name
     * @returns {string} Font Awesome icon class
     */
    getWeatherIconClass(weatherName) {
        const iconMap = {
            'Sunny': 'fas fa-sun',
            'Cloudy': 'fas fa-cloud',
            'Rainy': 'fas fa-cloud-rain',
            'Stormy': 'fas fa-bolt',
            'Foggy': 'fas fa-smog'
        };
        return iconMap[weatherName] || 'fas fa-question';
    }

    /**
     * Update predictions page
     */
    async updatePredictions() {
        try {
            const latest = await api.getLatestData();
            const data = await api.getHistoricalData(60);
            
            // Update current prediction
            const weather = api.getWeatherClass(latest.prediction);
            
            // Check if prediction changed
            if (this.lastPrediction !== null && this.lastPrediction !== latest.prediction) {
                const oldWeather = api.getWeatherClass(this.lastPrediction);
                this.addLogEntry('warning', `Prediction changed: ${oldWeather.name} → ${weather.name}`, {
                    'Previous': oldWeather.name,
                    'Current': weather.name,
                    'Confidence': '95%'
                });
                
                // Highlight the prediction card
                const predictionCard = document.querySelector('.prediction-detail-card');
                if (predictionCard) {
                    predictionCard.style.animation = 'pulse 0.5s ease';
                    setTimeout(() => {
                        predictionCard.style.animation = '';
                    }, 500);
                }
            }
            
            this.lastPrediction = latest.prediction;
            
            this.updateElement('mlWeatherName', weather.name);
            this.updateMLWeatherIcon(weather);
            this.updateElement('mlConfidence', '95%'); // Placeholder
            this.updateElement('mlInferenceTime', `${latest.inference.toLocaleString()} µs`);
            
            // Update input features
            this.updateElement('mlTemp', `${latest.temperature.toFixed(2)} °C`);
            this.updateElement('mlHumid', `${latest.humidity.toFixed(2)} %`);
            this.updateElement('mlPressure', `${latest.pressure.toFixed(0)} Pa`);
            this.updateElement('mlLux', `${latest.lux.toFixed(2)} lux`);
            this.updateElement('mlGas', `${latest.gas.toFixed(1)} PPM`);
            
            // Update prediction history chart
            if (data.length > 0) {
                chartManager.updatePredictionHistory(data);
            }
            
        } catch (error) {
            console.error('Error updating predictions:', error);
        }
    }

    /**
     * Update device status page
     */
    async updateDeviceStatus() {
        try {
            const latest = await api.getLatestData();
            
            // Update average inference time
            this.updateElement('avgInference', `${(latest.inference / 1000).toFixed(2)} ms`);
            
            // Calculate uptime (placeholder)
            const uptimeHours = Math.floor(Math.random() * 72) + 1;
            this.updateElement('systemUptime', uptimeHours);
            
        } catch (error) {
            console.error('Error updating device status:', error);
        }
    }

    /**
     * Update WiFi status page
     */
    async updateWiFiStatus() {
        try {
            const latest = await api.getLatestData();
            const wifiQuality = api.getWiFiQuality(latest.rssi);
            
            // Update signal strength
            this.updateElement('signalValue', `${latest.rssi} dBm`);
            this.updateElement('signalQuality', wifiQuality.level);
            
            // Update signal bars
            const signalBars = document.getElementById('signalBars');
            if (signalBars) {
                signalBars.className = 'signal-bars ' + wifiQuality.level.toLowerCase().replace(' ', '-');
            }
            
            // Update WiFi status
            this.updateElement('wifiState', 'Connected');
            this.updateElement('thingspeakStatus', '✅ Connected');
            this.updateElement('cloudThingspeakStatus', '✅ Connected');
            this.updateElement('lastUploadTime', api.formatTime(latest.timestamp));
            
            const stats = await api.getChannelStats();
            if (stats) {
                this.updateElement('uploadCount', stats.totalRecords.toLocaleString());
            }
            
            // Update Cloud Services section
            if (CONFIG.firebase && CONFIG.firebase.enabled) {
                this.updateElement('firebaseStatus', '✅ Connected');
                this.updateElement('firebaseProjectId', CONFIG.firebase.config.projectId || '--');
                
                // Extract region from database URL
                const dbUrl = CONFIG.firebase.config.databaseURL || '';
                const region = dbUrl.includes('asia-southeast1') ? 'Asia Southeast (Singapore)' : 
                              dbUrl.includes('us-central') ? 'US Central' : 
                              dbUrl.includes('europe-west') ? 'Europe West' : 'Default';
                this.updateElement('firebaseRegion', region);
            } else {
                this.updateElement('firebaseStatus', '❌ Disabled');
                this.updateElement('firebaseProjectId', 'Not configured');
                this.updateElement('firebaseRegion', '--');
            }
            
        } catch (error) {
            console.error('Error updating WiFi status:', error);
            this.updateElement('firebaseStatus', '⚠️ Error');
            this.updateElement('cloudThingspeakStatus', '⚠️ Error');
        }
    }

    /**
     * Update Activity Log page
     */
    async updateActivityLog() {
        try {
            const data = await api.getHistoricalData(60);
            
            if (data.length > 0) {
                const latest = data[data.length - 1];
                const stats = await api.getChannelStats();
                
                // Update stats
                if (stats) {
                    this.updateElement('activityTotalEntries', stats.totalRecords.toLocaleString());
                }
                
                // Calculate update rate (entries per minute)
                const updateRate = data.length > 1 ? (data.length / 60).toFixed(1) : '0';
                this.updateElement('activityUpdateRate', updateRate);
                this.updateElement('activityLastUpload', api.formatTime(latest.timestamp));
            }
            
        } catch (error) {
            console.error('Error updating activity log:', error);
        }
    }

    /**
     * Add entry to activity log
     * @param {string} type - Entry type: success, error, warning, info
     * @param {string} message - Log message
     * @param {Object} data - Optional data object
     */
    addLogEntry(type, message, data = null) {
        const logContainer = document.getElementById('activityLog');
        if (!logContainer) return;
        
        const now = new Date();
        const timeStr = now.toLocaleTimeString('en-US', { hour12: false });
        
        const entry = document.createElement('div');
        entry.className = `log-entry ${type}`;
        
        let iconClass = 'fa-info-circle';
        switch (type) {
            case 'success': iconClass = 'fa-check-circle'; break;
            case 'error': iconClass = 'fa-times-circle'; break;
            case 'warning': iconClass = 'fa-exclamation-triangle'; break;
        }
        
        let html = `
            <div class="log-time">${timeStr}</div>
            <div class="log-icon"><i class="fas ${iconClass}"></i></div>
            <div class="log-message">${message}`;
        
        if (data) {
            html += `<div class="log-data">`;
            for (const [key, value] of Object.entries(data)) {
                html += `
                    <div class="log-data-item">
                        <span class="log-data-label">${key}:</span>
                        <span class="log-data-value">${value}</span>
                    </div>`;
            }
            html += `</div>`;
        }
        
        html += `</div>`;
        entry.innerHTML = html;
        
        // Add to log
        logContainer.appendChild(entry);
        
        // Keep only last 100 entries
        const entries = logContainer.querySelectorAll('.log-entry');
        if (entries.length > 100) {
            entries[0].remove();
        }
        
        // Auto-scroll if enabled
        if (this.autoScroll) {
            logContainer.scrollTop = logContainer.scrollHeight;
        }
        
        // Store in memory
        this.activityLog.push({ time: now, type, message, data });
        if (this.activityLog.length > 100) {
            this.activityLog.shift();
        }
    }

    /**
     * Clear activity log
     */
    clearActivityLog() {
        const logContainer = document.getElementById('activityLog');
        if (!logContainer) return;
        
        logContainer.innerHTML = `
            <div class="log-entry info">
                <div class="log-time">${new Date().toLocaleTimeString('en-US', { hour12: false })}</div>
                <div class="log-icon"><i class="fas fa-info-circle"></i></div>
                <div class="log-message">Log cleared</div>
            </div>`;
        
        this.activityLog = [];
    }

    /**
     * Update value with flash animation
     * @param {string} id - Element ID
     * @param {string} value - New value
     * @param {boolean} animate - Whether to animate
     */
    updateValueWithFlash(id, value, animate = true) {
        const element = document.getElementById(id);
        if (!element) return;
        
        if (element.textContent !== value) {
            element.textContent = value;
            if (animate) {
                element.classList.add('flash');
                setTimeout(() => element.classList.remove('flash'), 500);
            }
        }
    }

    /**
     * Pulse all stat cards
     */
    pulseCards() {
        const cards = document.querySelectorAll('.stat-card');
        cards.forEach(card => {
            card.classList.add('updating');
            setTimeout(() => {
                card.classList.remove('updating');
                card.classList.add('updated');
                setTimeout(() => card.classList.remove('updated'), 2000);
            }, 600);
        });
    }

    /**
     * Set live indicator status
     * @param {boolean} isLive - Whether system is live
     */
    setLiveIndicator(isLive) {
        const indicator = document.getElementById('liveIndicator');
        if (!indicator) return;
        
        if (isLive) {
            indicator.classList.remove('offline');
            indicator.querySelector('span:last-child').textContent = 'LIVE';
        } else {
            indicator.classList.add('offline');
            indicator.querySelector('span:last-child').textContent = 'OFFLINE';
        }
    }

    /**
     * Export data to CSV
     */
    async exportData() {
        try {
            const data = await api.getHistoricalData(this.timeRange);
            const timestamp = new Date().toISOString().split('T')[0];
            api.exportToCSV(data, `weather_data_${timestamp}.csv`);
        } catch (error) {
            console.error('Error exporting data:', error);
            alert('Failed to export data. Please try again.');
        }
    }

    /**
     * Refresh current page data
     */
    async refreshCurrentPage() {
        const btn = document.getElementById('refreshBtn');
        if (btn) {
            btn.querySelector('i').style.animation = 'rotate 0.5s ease';
            setTimeout(() => {
                btn.querySelector('i').style.animation = '';
            }, 500);
        }
        
        await this.loadPageData(this.currentPage);
    }

    /**
     * Start auto-refresh for current page
     */
    startAutoRefresh() {
        this.stopAutoRefresh(); // Clear existing intervals
        
        if (!this.autoRefresh) return;
        
        // Set up intervals based on page
        this.refreshIntervals.dashboard = setInterval(() => {
            if (this.currentPage === 'dashboard') {
                this.updateDashboard();
            }
        }, CONFIG.updateIntervals.dashboard);
        
        this.refreshIntervals.graphs = setInterval(() => {
            if (this.currentPage === 'graphs') {
                this.updateGraphs();
            }
        }, CONFIG.updateIntervals.graphs);
        
        this.refreshIntervals.predictions = setInterval(() => {
            if (this.currentPage === 'predictions') {
                this.updatePredictions();
            }
        }, CONFIG.updateIntervals.predictions);
        
        this.refreshIntervals.activity = setInterval(() => {
            if (this.currentPage === 'activity') {
                this.updateActivityLog();
            }
        }, CONFIG.updateIntervals.activity);
        
        // Update real weather every 5 minutes (separate from page refresh)
        if (CONFIG.openWeatherMap.enabled) {
            this.refreshIntervals.weather = setInterval(() => {
                if (this.currentPage === 'dashboard') {
                    this.updateRealWeather();
                }
            }, CONFIG.openWeatherMap.updateInterval);
        }
    }

    /**
     * Stop auto-refresh
     */
    stopAutoRefresh() {
        Object.values(this.refreshIntervals).forEach(interval => {
            if (interval) clearInterval(interval);
        });
        this.refreshIntervals = {};
    }

    /**
     * Update element text content
     * @param {string} id - Element ID
     * @param {string} value - New value
     */
    updateElement(id, value) {
        const element = document.getElementById(id);
        if (element) {
            element.textContent = value;
        }
    }

    /**
     * Update trend indicator
     * @param {string} id - Element ID
     * @param {string} direction - 'up' or 'down'
     */
    updateTrend(id, direction) {
        const element = document.getElementById(id);
        if (!element) return;
        
        const icon = element.querySelector('i');
        if (direction === 'up') {
            icon.className = 'fas fa-arrow-up';
            element.classList.remove('down');
        } else {
            icon.className = 'fas fa-arrow-down';
            element.classList.add('down');
        }
    }

    /**
     * Update prediction icon
     * @param {Object} weather - Weather class info
     */
    updatePredictionIcon(weather) {
        const icon = document.getElementById('predictionIcon');
        if (!icon) return;
        
        const i = icon.querySelector('i');
        i.className = `fas ${weather.icon}`;
        icon.parentElement.parentElement.style.background = 
            `linear-gradient(135deg, ${weather.color}, ${this.adjustColor(weather.color, -20)})`;
    }

    /**
     * Update ML weather icon
     * @param {Object} weather - Weather class info
     */
    updateMLWeatherIcon(weather) {
        const icon = document.getElementById('mlWeatherIcon');
        if (!icon) return;
        
        const i = icon.querySelector('i');
        i.className = `fas ${weather.icon}`;
        icon.style.background = `linear-gradient(135deg, ${weather.color}, ${this.adjustColor(weather.color, -20)})`;
    }

    /**
     * Update last update time
     * @param {Date} timestamp - Timestamp
     */
    updateLastUpdateTime(timestamp) {
        const element = document.getElementById('lastUpdate');
        if (!element) return;
        
        const span = element.querySelector('span');
        span.textContent = `Last update: ${api.formatTime(timestamp)}`;
    }

    /**
     * Update backup & analysis page
     */
    async updateBackupPage() {
        try {
            // Initialize backup page if not already done
            if (typeof backupPage !== 'undefined' && backupPage) {
                await backupPage.init();
            } else {
                console.error('Backup page module not loaded');
            }
        } catch (error) {
            console.error('Error updating backup page:', error);
        }
    }

    /**
     * Set connection status
     * @param {string} status - 'connected', 'connecting', or 'disconnected'
     */
    setConnectionStatus(status) {
        const element = document.getElementById('connectionStatus');
        if (!element) return;
        
        const span = element.querySelector('span');
        
        element.className = 'connection-status';
        
        switch (status) {
            case 'connected':
                element.classList.add('connected');
                span.textContent = 'Connected';
                break;
            case 'connecting':
                span.textContent = 'Connecting...';
                break;
            case 'disconnected':
                element.classList.add('disconnected');
                span.textContent = 'Disconnected';
                break;
        }
    }

    /**
     * Adjust color brightness
     * @param {string} color - Hex color
     * @param {number} amount - Amount to adjust (-255 to 255)
     * @returns {string} Adjusted hex color
     */
    adjustColor(color, amount) {
        const clamp = (val) => Math.min(Math.max(val, 0), 255);
        const num = parseInt(color.slice(1), 16);
        const r = clamp((num >> 16) + amount);
        const g = clamp(((num >> 8) & 0x00FF) + amount);
        const b = clamp((num & 0x0000FF) + amount);
        return '#' + ((r << 16) | (g << 8) | b).toString(16).padStart(6, '0');
    }
}

document.addEventListener('DOMContentLoaded', () => {
    window.dashboard = new WeatherDashboard();
});
