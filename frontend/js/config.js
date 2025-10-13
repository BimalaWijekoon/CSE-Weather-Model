// ThingSpeak API Configuration
const CONFIG = {
    // ThingSpeak Channel Settings
    channelId: '3108323',
    readApiKey: '6RV3OE2SHGO9YPJ8', // Read API key for private channel
    writeApiKey: 'J3GFLQKI0TVR6JC2',
    
    // API Endpoints
    apiUrl: 'https://api.thingspeak.com',
    
    // OpenWeatherMap API Configuration
    openWeatherMap: {
        apiKey: 'fa326e062a972cf227f187e6740628f5',
        apiUrl: 'https://api.openweathermap.org/data/2.5',
        enabled: true, // Set to true after adding API key
        updateInterval: 300000 // 5 minutes (free tier: 60 calls/minute, 1000/day)
    },
    
    // Geolocation Settings
    geolocation: {
        enabled: true,
        defaultLat: 0,
        defaultLon: 0,
        defaultCity: 'Unknown'
    },
    
    // Field Mapping
    fields: {
        temperature: 1,
        humidity: 2,
        pressure: 3,
        lux: 4,
        gas: 5,
        prediction: 6,
        inference: 7,
        rssi: 8
    },
    
    // Weather Class Mapping
    weatherClasses: [
        { id: 0, name: 'Cloudy', icon: 'fa-cloud', color: '#94a3b8' },
        { id: 1, name: 'Foggy', icon: 'fa-smog', color: '#67e8f9' },
        { id: 2, name: 'Rainy', icon: 'fa-cloud-rain', color: '#3b82f6' },
        { id: 3, name: 'Stormy', icon: 'fa-bolt', color: '#8b5cf6' },
        { id: 4, name: 'Sunny', icon: 'fa-sun', color: '#fbbf24' }
    ],
    
    // Update Intervals (milliseconds)
    updateIntervals: {
        dashboard: 2000,       // 2 seconds - FASTER REAL-TIME
        graphs: 5000,          // 5 seconds (was 10)
        predictions: 2000,     // 2 seconds - FASTER for prediction changes
        device: 30000,         // 30 seconds
        activity: 2000         // 2 seconds - Faster activity log
    },
    
    // Data Points for Graphs
    graphDataPoints: {
        '60': 60,      // 1 hour (every minute)
        '360': 72,     // 6 hours (every 5 minutes)
        '1440': 96     // 24 hours (every 15 minutes)
    },
    
    // Gas Sensor Thresholds (PPM)
    gasThresholds: {
        good: 50,
        moderate: 100,
        unhealthy: 150
    },
    
    // WiFi Signal Quality (dBm)
    wifiQuality: {
        excellent: -50,
        good: -60,
        fair: -70,
        poor: -80
    },
    
    // Firebase Configuration
    firebase: {
        enabled: true,
        config: {
            apiKey: "AIzaSyDO5pK4EIi6N4n9PaZZ4GvS7wh52aPsiyU",
            authDomain: "weather-prediction-syste-9d94d.firebaseapp.com",
            databaseURL: "https://weather-prediction-syste-9d94d-default-rtdb.asia-southeast1.firebasedatabase.app",
            projectId: "weather-prediction-syste-9d94d",
            storageBucket: "weather-prediction-syste-9d94d.firebasestorage.app",
            messagingSenderId: "560109147497",
            appId: "1:560109147497:web:ef06ff5d4d28448ef8d9f8",
            measurementId: "G-1JZRSJM8ZS"
        },
        // Authentication credentials (must match ESP32 credentials)
        auth: {
            email: "esp32@test.com",
            password: "esp32test123"
        },
        // Default device ID (will be auto-detected from available devices)
        defaultDeviceId: null
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = CONFIG;
}

// Create lowercase alias for compatibility
const config = CONFIG;
