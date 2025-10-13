// Backup & Analysis Page Handler
class BackupPage {
    constructor() {
        this.currentDevice = null;
        this.allReadings = [];
        this.filteredReadings = [];
        this.currentPage = 1;
        this.itemsPerPage = 20;
        this.charts = {
            weatherDistribution: null,
            sensorTrends: null
        };
    }

    /**
     * Initialize the backup page
     */
    async init() {
        console.log('Initializing Backup & Analysis page...');
        
        // Initialize Firebase if not already done
        if (!firebaseAPI.initialized) {
            const success = await firebaseAPI.initialize();
            if (!success) {
                this.showError('Firebase initialization failed. Check configuration and authentication credentials.');
                return;
            }
        }

        // Check authentication
        if (!firebaseAPI.authenticated) {
            this.showError('Firebase authentication required. Please check email/password in config.js match Firebase Authentication users.');
            return;
        }

        // Load devices
        await this.loadDevices();
        
        // Setup event listeners
        this.setupEventListeners();
        
        // Load initial data if device is available
        if (this.currentDevice) {
            await this.loadBackupData();
        }
    }

    /**
     * Load available devices from Firebase
     */
    async loadDevices() {
        try {
            const devices = await firebaseAPI.getDevices();
            const deviceSelector = document.getElementById('deviceSelector');
            
            if (!deviceSelector) return;
            
            if (devices.length === 0) {
                deviceSelector.innerHTML = '<option value="">No devices found</option>';
                this.showError('No devices found in Firebase. Please check if ESP32 has uploaded data.');
                return;
            }

            // Populate device selector
            deviceSelector.innerHTML = devices.map(device => {
                return `<option value="${device.id}">${device.id}</option>`;
            }).join('');

            // Set first device as current
            this.currentDevice = devices[0].id;
            firebaseAPI.setDevice(this.currentDevice);
            
            console.log('Devices loaded:', devices.length);
        } catch (error) {
            console.error('Error loading devices:', error);
            this.showError('Failed to load devices from Firebase.');
        }
    }

    /**
     * Setup event listeners for controls
     */
    setupEventListeners() {
        // Device selector
        const deviceSelector = document.getElementById('deviceSelector');
        if (deviceSelector) {
            deviceSelector.addEventListener('change', async (e) => {
                this.currentDevice = e.target.value;
                firebaseAPI.setDevice(this.currentDevice);
                await this.loadBackupData();
            });
        }

        // Time range selector
        const timeRangeSelector = document.getElementById('timeRangeSelector');
        if (timeRangeSelector) {
            timeRangeSelector.addEventListener('change', async () => {
                await this.loadBackupData();
            });
        }

        // Weather filter
        const weatherFilter = document.getElementById('weatherFilter');
        if (weatherFilter) {
            weatherFilter.addEventListener('change', () => {
                this.applyFilters();
            });
        }

        // Refresh button
        const refreshBtn = document.getElementById('refreshBackupData');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', async () => {
                refreshBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Loading...';
                await this.loadBackupData();
                refreshBtn.innerHTML = '<i class="fas fa-sync-alt"></i> Refresh Data';
            });
        }

        // Export buttons
        const exportCSV = document.getElementById('exportCSV');
        if (exportCSV) {
            exportCSV.addEventListener('click', () => {
                firebaseAPI.downloadCSV(this.filteredReadings);
            });
        }

        const exportJSON = document.getElementById('exportJSON');
        if (exportJSON) {
            exportJSON.addEventListener('click', () => {
                firebaseAPI.downloadJSON(this.filteredReadings);
            });
        }

        // Pagination buttons
        const prevPage = document.getElementById('prevPage');
        const nextPage = document.getElementById('nextPage');
        
        if (prevPage) {
            prevPage.addEventListener('click', () => {
                if (this.currentPage > 1) {
                    this.currentPage--;
                    this.renderTable();
                }
            });
        }

        if (nextPage) {
            nextPage.addEventListener('click', () => {
                const totalPages = Math.ceil(this.filteredReadings.length / this.itemsPerPage);
                if (this.currentPage < totalPages) {
                    this.currentPage++;
                    this.renderTable();
                }
            });
        }
    }

    /**
     * Load backup data from Firebase
     */
    async loadBackupData() {
        if (!this.currentDevice) {
            this.showError('No device selected');
            return;
        }

        try {
            // Show loading state
            this.showLoading();

            // Get time range limit
            const timeRange = document.getElementById('timeRangeSelector')?.value || '50';
            const limit = timeRange === 'all' ? null : parseInt(timeRange);

            // Fetch readings
            this.allReadings = await firebaseAPI.getAllReadings(this.currentDevice, limit);
            
            if (this.allReadings.length === 0) {
                this.showError('No backup data found for this device. Please wait for ESP32 to upload data.');
                return;
            }

            // Apply filters
            this.applyFilters();

            // Load statistics
            await this.loadStatistics();

            // Load device info
            await this.loadDeviceInfo();

            // Render charts
            this.renderCharts();

            // Render table
            this.currentPage = 1;
            this.renderTable();

            console.log('Backup data loaded:', this.allReadings.length, 'readings');
        } catch (error) {
            console.error('Error loading backup data:', error);
            this.showError('Failed to load backup data. Please try again.');
        }
    }

    /**
     * Apply weather filter
     */
    applyFilters() {
        const weatherFilter = document.getElementById('weatherFilter')?.value || 'all';
        this.filteredReadings = firebaseAPI.filterByWeather(this.allReadings, weatherFilter);
        
        // Reset to first page
        this.currentPage = 1;
        this.renderTable();
        this.renderCharts();
    }

    /**
     * Load and display statistics
     */
    async loadStatistics() {
        try {
            const stats = await firebaseAPI.getReadingsStats(this.currentDevice);
            
            // Update stats cards
            this.updateElement('backupTotalReadings', stats.total.toLocaleString());
            this.updateElement('backupDeviceCount', '1'); // Current implementation supports single device
            this.updateElement('backupTodayCount', stats.today.toLocaleString());
            this.updateElement('backupAvgPerHour', stats.avgPerHour);
        } catch (error) {
            console.error('Error loading statistics:', error);
        }
    }

    /**
     * Load and display device information
     */
    async loadDeviceInfo() {
        try {
            const info = await firebaseAPI.getDeviceInfo(this.currentDevice);
            const status = await firebaseAPI.getDeviceStatus(this.currentDevice);
            
            console.log('Device Info:', info);
            console.log('Device Status:', status);
            
            // Update device information
            this.updateElement('backupDeviceId', this.currentDevice);
            this.updateElement('backupFirmware', info.firmware_version || '--');
            this.updateElement('backupModelType', info.model_type || '--');
            this.updateElement('backupChipModel', info.chip_model || '--');
            this.updateElement('backupMacAddress', info.mac_address || '--');
            
            // last_seen is seconds since boot, not Unix timestamp
            if (status.last_seen && status.online) {
                // If online, show as "Just now" or relative time
                const secondsAgo = status.last_seen;
                if (secondsAgo < 60) {
                    this.updateElement('backupLastSeen', 'Just now');
                } else if (secondsAgo < 3600) {
                    this.updateElement('backupLastSeen', `${Math.floor(secondsAgo / 60)} min ago (uptime)`);
                } else {
                    this.updateElement('backupLastSeen', `${Math.floor(secondsAgo / 3600)} hrs ago (uptime)`);
                }
            } else if (status.last_seen) {
                this.updateElement('backupLastSeen', `Uptime: ${Math.floor(status.last_seen / 60)} minutes`);
            } else {
                this.updateElement('backupLastSeen', 'Never');
            }

            // Update status badge
            const statusBadge = document.getElementById('backupDeviceStatus');
            if (statusBadge) {
                if (status.online) {
                    statusBadge.className = 'detail-value status-badge online';
                    statusBadge.innerHTML = '<span class="status-dot"></span> Online';
                } else {
                    statusBadge.className = 'detail-value status-badge offline';
                    statusBadge.innerHTML = '<span class="status-dot"></span> Offline';
                }
            }
        } catch (error) {
            console.error('Error loading device info:', error);
        }
    }

    /**
     * Render data table with pagination
     */
    renderTable() {
        const tbody = document.getElementById('backupTableBody');
        if (!tbody) return;

        if (this.filteredReadings.length === 0) {
            tbody.innerHTML = '<tr><td colspan="7" class="loading-cell">No data available</td></tr>';
            return;
        }

        // Calculate pagination
        const startIdx = (this.currentPage - 1) * this.itemsPerPage;
        const endIdx = startIdx + this.itemsPerPage;
        const pageReadings = this.filteredReadings.slice(startIdx, endIdx);

        // Render rows
        tbody.innerHTML = pageReadings.map((reading, index) => {
            // Use relative time display since timestamp is seconds since boot
            const timeAgo = this.filteredReadings.length - (startIdx + index);
            const timeLabel = timeAgo === 1 ? 'Latest' : `${timeAgo} readings ago`;
            const weatherClass = (reading.prediction || 'unknown').toLowerCase();
            
            return `
                <tr>
                    <td>${timeLabel}</td>
                    <td>${reading.temperature?.toFixed(2) || '--'}</td>
                    <td>${reading.humidity?.toFixed(2) || '--'}</td>
                    <td>${reading.pressure?.toFixed(2) || '--'}</td>
                    <td>${reading.lux?.toFixed(2) || '--'}</td>
                    <td>
                        <span class="weather-badge ${weatherClass}">
                            ${reading.prediction || 'Unknown'}
                        </span>
                    </td>
                    <td>${(reading.inference_time || 0).toLocaleString()}</td>
                </tr>
            `;
        }).join('');

        // Update pagination controls
        this.updatePagination();
    }

    /**
     * Update pagination controls
     */
    updatePagination() {
        const totalPages = Math.ceil(this.filteredReadings.length / this.itemsPerPage);
        
        const prevBtn = document.getElementById('prevPage');
        const nextBtn = document.getElementById('nextPage');
        const paginationInfo = document.getElementById('paginationInfo');

        if (prevBtn) {
            prevBtn.disabled = this.currentPage === 1;
        }

        if (nextBtn) {
            nextBtn.disabled = this.currentPage >= totalPages;
        }

        if (paginationInfo) {
            paginationInfo.textContent = `Page ${this.currentPage} of ${totalPages || 1}`;
        }
    }

    /**
     * Render charts
     */
    renderCharts() {
        this.renderWeatherDistributionChart();
        this.renderSensorTrendsChart();
    }

    /**
     * Render weather distribution chart
     */
    renderWeatherDistributionChart() {
        const canvas = document.getElementById('weatherDistributionChart');
        if (!canvas) return;

        // Count weather types
        const weatherCounts = {};
        this.filteredReadings.forEach(r => {
            const weather = r.prediction || 'Unknown';
            weatherCounts[weather] = (weatherCounts[weather] || 0) + 1;
        });

        // Prepare data
        const labels = Object.keys(weatherCounts);
        const data = Object.values(weatherCounts);
        const colors = labels.map(label => {
            const weather = label.toLowerCase();
            if (weather === 'sunny') return '#fbbf24';
            if (weather === 'cloudy') return '#94a3b8';
            if (weather === 'rainy') return '#3b82f6';
            if (weather === 'stormy') return '#8b5cf6';
            if (weather === 'foggy') return '#67e8f9';
            return '#666666';
        });

        // Destroy existing chart
        if (this.charts.weatherDistribution) {
            this.charts.weatherDistribution.destroy();
        }

        // Create new chart
        this.charts.weatherDistribution = new Chart(canvas, {
            type: 'doughnut',
            data: {
                labels: labels,
                datasets: [{
                    data: data,
                    backgroundColor: colors,
                    borderWidth: 2,
                    borderColor: getComputedStyle(document.documentElement).getPropertyValue('--bg-card')
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                layout: {
                    padding: {
                        bottom: 20
                    }
                },
                plugins: {
                    legend: {
                        position: 'bottom',
                        align: 'center',
                        labels: {
                            color: getComputedStyle(document.documentElement).getPropertyValue('--text-primary'),
                            padding: 12,
                            boxWidth: 15,
                            boxHeight: 15,
                            font: { size: 11, weight: '600' }
                        }
                    },
                    tooltip: {
                        backgroundColor: 'rgba(0, 0, 0, 0.8)',
                        padding: 12,
                        cornerRadius: 8,
                        titleColor: '#fff',
                        bodyColor: '#fff',
                        callbacks: {
                            label: function(context) {
                                const total = context.dataset.data.reduce((a, b) => a + b, 0);
                                const percentage = ((context.parsed / total) * 100).toFixed(1);
                                return `${context.label}: ${context.parsed} (${percentage}%)`;
                            }
                        }
                    }
                }
            }
        });
    }

    /**
     * Render sensor trends chart
     */
    renderSensorTrendsChart() {
        const canvas = document.getElementById('sensorTrendsChart');
        if (!canvas) return;

        // Group readings by hour or limit to last 20 points
        const dataPoints = this.filteredReadings.slice(0, 20).reverse();
        
        const labels = dataPoints.map(r => {
            const date = new Date(r.timestamp * 1000);
            return date.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
        });

        // Destroy existing chart
        if (this.charts.sensorTrends) {
            this.charts.sensorTrends.destroy();
        }

        // Create new chart
        this.charts.sensorTrends = new Chart(canvas, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    {
                        label: 'Temperature (°C)',
                        data: dataPoints.map(r => r.temperature),
                        borderColor: '#ef4444',
                        backgroundColor: 'rgba(239, 68, 68, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true,
                        pointRadius: 3,
                        pointHoverRadius: 5,
                        pointBackgroundColor: '#ef4444',
                        pointBorderColor: '#fff',
                        pointBorderWidth: 2,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Humidity (%)',
                        data: dataPoints.map(r => r.humidity),
                        borderColor: '#3b82f6',
                        backgroundColor: 'rgba(59, 130, 246, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true,
                        pointRadius: 3,
                        pointHoverRadius: 5,
                        pointBackgroundColor: '#3b82f6',
                        pointBorderColor: '#fff',
                        pointBorderWidth: 2,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Pressure (Pa)',
                        data: dataPoints.map(r => r.pressure),
                        borderColor: '#8b5cf6',
                        backgroundColor: 'rgba(139, 92, 246, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true,
                        pointRadius: 3,
                        pointHoverRadius: 5,
                        pointBackgroundColor: '#8b5cf6',
                        pointBorderColor: '#fff',
                        pointBorderWidth: 2,
                        yAxisID: 'y1'
                    },
                    {
                        label: 'Lux',
                        data: dataPoints.map(r => r.lux),
                        borderColor: '#fbbf24',
                        backgroundColor: 'rgba(251, 191, 36, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true,
                        pointRadius: 3,
                        pointHoverRadius: 5,
                        pointBackgroundColor: '#fbbf24',
                        pointBorderColor: '#fff',
                        pointBorderWidth: 2,
                        yAxisID: 'y2'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                layout: {
                    padding: {
                        bottom: 10
                    }
                },
                interaction: {
                    mode: 'index',
                    intersect: false
                },
                plugins: {
                    legend: {
                        position: 'bottom',
                        align: 'center',
                        labels: {
                            color: getComputedStyle(document.documentElement).getPropertyValue('--text-primary'),
                            padding: 10,
                            boxWidth: 12,
                            boxHeight: 12,
                            font: { size: 11, weight: '600' },
                            usePointStyle: true,
                            pointStyle: 'circle'
                        }
                    },
                    tooltip: {
                        backgroundColor: 'rgba(0, 0, 0, 0.8)',
                        padding: 12,
                        cornerRadius: 8,
                        titleColor: '#fff',
                        bodyColor: '#fff',
                        borderColor: 'rgba(255, 255, 255, 0.2)',
                        borderWidth: 1
                    }
                },
                scales: {
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: {
                            display: true,
                            text: 'Temp/Humidity',
                            color: getComputedStyle(document.documentElement).getPropertyValue('--text-secondary')
                        },
                        grid: {
                            color: getComputedStyle(document.documentElement).getPropertyValue('--border-color')
                        },
                        ticks: {
                            color: getComputedStyle(document.documentElement).getPropertyValue('--text-secondary')
                        }
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: {
                            display: true,
                            text: 'Pressure (Pa)',
                            color: getComputedStyle(document.documentElement).getPropertyValue('--text-secondary')
                        },
                        grid: {
                            drawOnChartArea: false
                        },
                        ticks: {
                            color: getComputedStyle(document.documentElement).getPropertyValue('--text-secondary')
                        }
                    },
                    y2: {
                        type: 'linear',
                        display: false,
                        position: 'right',
                        grid: {
                            drawOnChartArea: false
                        }
                    },
                    x: {
                        grid: {
                            display: false
                        },
                        ticks: {
                            color: getComputedStyle(document.documentElement).getPropertyValue('--text-secondary'),
                            maxRotation: 45,
                            minRotation: 45
                        }
                    }
                }
            }
        });
    }

    /**
     * Show loading state
     */
    showLoading() {
        const tbody = document.getElementById('backupTableBody');
        if (tbody) {
            tbody.innerHTML = '<tr><td colspan="7" class="loading-cell"><i class="fas fa-spinner fa-spin"></i> Loading data...</td></tr>';
        }
    }

    /**
     * Show error message
     */
    showError(message) {
        const tbody = document.getElementById('backupTableBody');
        if (tbody) {
            tbody.innerHTML = `<tr><td colspan="7" class="loading-cell"><i class="fas fa-exclamation-triangle"></i> ${message}</td></tr>`;
        }
        console.error('Backup page error:', message);
    }

    /**
     * Update element content safely
     */
    updateElement(id, value) {
        const element = document.getElementById(id);
        if (element) {
            element.textContent = value;
        }
    }

    /**
     * Cleanup when leaving page
     */
    cleanup() {
        // Destroy charts
        if (this.charts.weatherDistribution) {
            this.charts.weatherDistribution.destroy();
            this.charts.weatherDistribution = null;
        }
        if (this.charts.sensorTrends) {
            this.charts.sensorTrends.destroy();
            this.charts.sensorTrends = null;
        }
    }
}

// Create global instance
const backupPage = new BackupPage();
