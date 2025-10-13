// Chart.js Configuration and Management
class ChartManager {
    constructor() {
        this.charts = {};
        this.sparklines = {};
        this.defaultColors = {
            temperature: '#ef4444',
            humidity: '#3b82f6',
            pressure: '#8b5cf6',
            lux: '#fbbf24',
            gas: '#10b981'
        };
    }

    /**
     * Initialize all charts
     */
    initializeCharts() {
        // Initialize main sensor charts
        this.createSensorChart('tempChart', 'Temperature', '°C', this.defaultColors.temperature);
        this.createSensorChart('humidChart', 'Humidity', '%', this.defaultColors.humidity);
        this.createSensorChart('pressureChart', 'Pressure', 'Pa', this.defaultColors.pressure);
        this.createSensorChart('luxChart', 'Light', 'lux', this.defaultColors.lux);
        this.createSensorChart('gasChart', 'Gas', 'PPM', this.defaultColors.gas);
        
        // Initialize prediction history chart
        this.createPredictionHistoryChart();
        
        // Initialize sparklines
        this.initializeSparklines();
    }

    /**
     * Create sensor chart
     * @param {string} canvasId - Canvas element ID
     * @param {string} label - Chart label
     * @param {string} unit - Data unit
     * @param {string} color - Line color
     */
    createSensorChart(canvasId, label, unit, color) {
        const canvas = document.getElementById(canvasId);
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        
        this.charts[canvasId] = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: label,
                    data: [],
                    borderColor: color,
                    backgroundColor: this.hexToRGBA(color, 0.1),
                    borderWidth: 2,
                    fill: true,
                    tension: 0.4,
                    pointRadius: 3,
                    pointHoverRadius: 5,
                    pointBackgroundColor: color,
                    pointBorderColor: '#fff',
                    pointBorderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    intersect: false,
                    mode: 'index'
                },
                plugins: {
                    legend: {
                        display: false
                    },
                    tooltip: {
                        backgroundColor: 'rgba(0, 0, 0, 0.8)',
                        padding: 12,
                        cornerRadius: 8,
                        titleColor: '#fff',
                        bodyColor: '#fff',
                        callbacks: {
                            label: (context) => {
                                return `${label}: ${context.parsed.y.toFixed(2)} ${unit}`;
                            }
                        }
                    }
                },
                scales: {
                    x: {
                        display: true,
                        grid: {
                            display: false
                        },
                        ticks: {
                            maxTicksLimit: 8,
                            color: getComputedStyle(document.documentElement)
                                .getPropertyValue('--text-secondary')
                        }
                    },
                    y: {
                        display: true,
                        grid: {
                            color: getComputedStyle(document.documentElement)
                                .getPropertyValue('--border-color')
                        },
                        ticks: {
                            color: getComputedStyle(document.documentElement)
                                .getPropertyValue('--text-secondary'),
                            callback: (value) => {
                                return value.toFixed(1) + ' ' + unit;
                            }
                        }
                    }
                }
            }
        });
    }

    /**
     * Create prediction history chart
     */
    createPredictionHistoryChart() {
        const canvas = document.getElementById('predictionHistoryChart');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        
        // Create gradient for line
        const gradient = ctx.createLinearGradient(0, 0, 0, 400);
        gradient.addColorStop(0, 'rgba(139, 92, 246, 0.3)');
        gradient.addColorStop(0.5, 'rgba(59, 130, 246, 0.2)');
        gradient.addColorStop(1, 'rgba(59, 130, 246, 0.05)');
        
        this.charts['predictionHistoryChart'] = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Weather Predictions',
                    data: [],
                    borderColor: '#8b5cf6',
                    backgroundColor: gradient,
                    borderWidth: 3,
                    tension: 0.4,
                    fill: true,
                    pointRadius: 5,
                    pointHoverRadius: 8,
                    pointBackgroundColor: '#8b5cf6',
                    pointBorderColor: '#ffffff',
                    pointBorderWidth: 3,
                    pointHoverBackgroundColor: '#ffffff',
                    pointHoverBorderColor: '#8b5cf6',
                    pointHoverBorderWidth: 3,
                    pointStyle: 'circle'
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    intersect: false,
                    mode: 'index'
                },
                plugins: {
                    legend: {
                        display: false
                    },
                    tooltip: {
                        backgroundColor: 'rgba(17, 24, 39, 0.95)',
                        titleColor: '#ffffff',
                        bodyColor: '#ffffff',
                        padding: 16,
                        cornerRadius: 12,
                        displayColors: false,
                        titleFont: {
                            size: 14,
                            weight: '600'
                        },
                        bodyFont: {
                            size: 16,
                            weight: '700'
                        },
                        callbacks: {
                            title: (context) => {
                                return `Time: ${context[0].label}`;
                            },
                            label: (context) => {
                                const predValue = Math.round(context.parsed.y);
                                const weather = CONFIG.weatherClasses[predValue];
                                const icon = weather ? this.getWeatherIcon(predValue) : '❓';
                                return weather ? `${icon} ${weather.name}` : 'Unknown';
                            },
                            afterLabel: (context) => {
                                const predValue = Math.round(context.parsed.y);
                                return `Class: ${predValue}`;
                            }
                        }
                    }
                },
                scales: {
                    x: {
                        display: true,
                        grid: {
                            display: true,
                            color: 'rgba(148, 163, 184, 0.1)',
                            drawBorder: false,
                            lineWidth: 1
                        },
                        ticks: {
                            color: '#94a3b8',
                            font: {
                                size: 11,
                                weight: '500'
                            },
                            maxRotation: 45,
                            minRotation: 45,
                            autoSkip: true,
                            maxTicksLimit: 12
                        },
                        border: {
                            display: false
                        }
                    },
                    y: {
                        display: true,
                        min: 0,
                        max: 4,
                        ticks: {
                            stepSize: 1,
                            color: '#94a3b8',
                            font: {
                                size: 12,
                                weight: '600'
                            },
                            callback: (value) => {
                                const weather = CONFIG.weatherClasses[value];
                                return weather ? weather.name : '';
                            },
                            padding: 10
                        },
                        grid: {
                            display: true,
                            color: 'rgba(148, 163, 184, 0.1)',
                            drawBorder: false,
                            lineWidth: 1
                        },
                        border: {
                            display: false,
                            dash: [5, 5]
                        }
                    }
                }
            }
        });
    }

    /**
     * Get weather icon emoji for tooltips
     */
    getWeatherIcon(classId) {
        const icons = {
            0: '☁️',  // Cloudy
            1: '🌫️',  // Foggy
            2: '🌧️',  // Rainy
            3: '⛈️',  // Stormy
            4: '☀️'   // Sunny
        };
        return icons[classId] || '❓';
    }

    /**
     * Initialize sparkline charts
     */
    initializeSparklines() {
        const sparklineIds = ['tempSparkline', 'humidSparkline', 'pressureSparkline', 'luxSparkline', 'gasSparkline'];
        const colors = [
            this.defaultColors.temperature,
            this.defaultColors.humidity,
            this.defaultColors.pressure,
            this.defaultColors.lux,
            this.defaultColors.gas
        ];

        sparklineIds.forEach((id, index) => {
            const canvas = document.getElementById(id);
            if (!canvas) return;

            const ctx = canvas.getContext('2d');
            
            this.sparklines[id] = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: Array(20).fill(''),
                    datasets: [{
                        data: Array(20).fill(0),
                        borderColor: colors[index],
                        backgroundColor: this.hexToRGBA(colors[index], 0.2),
                        borderWidth: 2,
                        fill: true,
                        tension: 0.4,
                        pointRadius: 0
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { display: false },
                        tooltip: { enabled: false }
                    },
                    scales: {
                        x: { display: false },
                        y: { display: false }
                    }
                }
            });
        });
    }

    /**
     * Update sensor chart with new data
     * @param {string} chartId - Chart ID
     * @param {Array} data - Historical data points
     * @param {string} field - Data field name
     */
    updateSensorChart(chartId, data, field) {
        const chart = this.charts[chartId];
        if (!chart || !data || data.length === 0) return;

        const labels = data.map(point => {
            const date = new Date(point.timestamp);
            return date.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
        });

        const values = data.map(point => point[field]);

        chart.data.labels = labels;
        chart.data.datasets[0].data = values;
        chart.update('none'); // Update without animation for performance
    }

    /**
     * Update prediction history chart
     * @param {Array} data - Historical data points
     */
    updatePredictionHistory(data) {
        const chart = this.charts['predictionHistoryChart'];
        if (!chart || !data || data.length === 0) return;

        // Get last 20 predictions for better trend visibility
        const recent = data.slice(-20);

        // Create labels with time stamps
        const labels = recent.map(point => {
            const date = new Date(point.timestamp);
            return date.toLocaleTimeString('en-US', { 
                hour: '2-digit', 
                minute: '2-digit',
                hour12: false 
            });
        });
        
        const predictions = recent.map(point => point.prediction);

        // Update chart data
        chart.data.labels = labels;
        chart.data.datasets[0].data = predictions;
        chart.update('none');
    }

    /**
     * Update sparkline with new data point
     * @param {string} sparklineId - Sparkline ID
     * @param {number} value - New value
     */
    updateSparkline(sparklineId, value) {
        const sparkline = this.sparklines[sparklineId];
        if (!sparkline) return;

        const data = sparkline.data.datasets[0].data;
        data.shift(); // Remove oldest
        data.push(value); // Add newest

        sparkline.update('none');
    }

    /**
     * Update all sparklines with latest data
     * @param {Object} latestData - Latest sensor readings
     */
    updateAllSparklines(latestData) {
        this.updateSparkline('tempSparkline', latestData.temperature);
        this.updateSparkline('humidSparkline', latestData.humidity);
        this.updateSparkline('pressureSparkline', latestData.pressure);
        this.updateSparkline('luxSparkline', latestData.lux);
        this.updateSparkline('gasSparkline', latestData.gas);
    }

    /**
     * Update all graphs with new data
     * @param {Array} historicalData - Historical data points
     */
    updateAllGraphs(historicalData) {
        if (!historicalData || historicalData.length === 0) return;

        this.updateSensorChart('tempChart', historicalData, 'temperature');
        this.updateSensorChart('humidChart', historicalData, 'humidity');
        this.updateSensorChart('pressureChart', historicalData, 'pressure');
        this.updateSensorChart('luxChart', historicalData, 'lux');
        this.updateSensorChart('gasChart', historicalData, 'gas');
        this.updatePredictionHistory(historicalData);
    }

    /**
     * Destroy all charts
     */
    destroyAll() {
        Object.values(this.charts).forEach(chart => {
            if (chart) chart.destroy();
        });
        Object.values(this.sparklines).forEach(chart => {
            if (chart) chart.destroy();
        });
        this.charts = {};
        this.sparklines = {};
    }

    /**
     * Convert hex color to RGBA
     * @param {string} hex - Hex color code
     * @param {number} alpha - Alpha value (0-1)
     * @returns {string} RGBA color string
     */
    hexToRGBA(hex, alpha = 1) {
        const r = parseInt(hex.slice(1, 3), 16);
        const g = parseInt(hex.slice(3, 5), 16);
        const b = parseInt(hex.slice(5, 7), 16);
        return `rgba(${r}, ${g}, ${b}, ${alpha})`;
    }

    /**
     * Update chart theme (for dark mode)
     */
    updateTheme() {
        const textColor = getComputedStyle(document.documentElement)
            .getPropertyValue('--text-secondary');
        const gridColor = getComputedStyle(document.documentElement)
            .getPropertyValue('--border-color');

        Object.values(this.charts).forEach(chart => {
            if (!chart.options.scales) return;
            
            // Update text colors
            if (chart.options.scales.x) {
                chart.options.scales.x.ticks.color = textColor;
            }
            if (chart.options.scales.y) {
                chart.options.scales.y.ticks.color = textColor;
                chart.options.scales.y.grid.color = gridColor;
            }
            
            chart.update('none');
        });
    }
}

// Create global chart manager instance
const chartManager = new ChartManager();
