# Firebase ESP32 Troubleshooting Guide

**Date:** October 13, 2025  
**Issue:** Firebase not uploading from ESP32-S3

---

## ✅ Credentials Fixed

### Corrected Configuration:

**Frontend (`frontend/js/config.js`):**
```javascript
firebase: {
    enabled: true,
    config: {
        apiKey: "AIzaSyDO5pK4EIi6N4n9PaZZ4GvS7wh52aPsiyU",
        authDomain: "weather-prediction-syste-9d94d.firebaseapp.com",
        databaseURL: "https://weather-prediction-syste-9d94d-default-rtdb.asia-southeast1.firebasedatabase.app",
        projectId: "weather-prediction-syste-9d94d",
        storageBucket: "weather-prediction-syste-9d94d.firebasestorage.app",
        messagingSenderId: "560109147497",     // ✅ FIXED
        appId: "1:560109147497:web:ef06ff5d4d28448ef8d9f8",  // ✅ FIXED
        measurementId: "G-1JZRSJM8ZS"          // ✅ ADDED
    }
}
```

**ESP32 (`firebase_manager.h`):**
```cpp
#define FIREBASE_HOST "weather-prediction-syste-9d94d-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_API_KEY "AIzaSyDO5pK4EIi6N4n9PaZZ4GvS7wh52aPsiyU"
```

✅ ESP32 credentials are correct (only needs API Key + Database URL)  
✅ Frontend credentials now match Firebase Console

---

## 🔍 Diagnostic Steps

### Step 1: Check Serial Monitor Output

Your current output shows:
```
15:58:19.543 -> 💡 Available Commands:
15:58:19.543 ->    • startsim   - Start continuous simulation (RECOMMENDED)
15:58:19.543 ->    • help       - Show command help
```

**What's missing:**
- No "STEP 3: Firebase Backup" message visible
- No Firebase connection status

**Possible reasons:**
1. Serial output was truncated/scrolled past
2. Need to **start simulation** with `startsim` command
3. Firebase initialization happened but wasn't captured

---

## 🚀 Action Plan

### Step 1: Reset ESP32 and Capture Full Boot Log

1. **Open Serial Monitor** (115200 baud)
2. **Press RESET button** on ESP32
3. **Watch for these messages:**

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
STEP 1: WiFi Connection
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   SSID: [Your Network]
   Status: ✅ Connected

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
STEP 2: ThingSpeak Cloud Connection
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   Status: ✅ Connection successful

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
STEP 3: Firebase Backup
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   Device ID: [MAC Address]
   Database: weather-prediction-syste-9d94d-default-rtdb.asia-southeast1.firebasedatabase.app
   Interval: 15 seconds
   Connecting...........
   Status: ✅ Connected to Firebase    <-- LOOK FOR THIS
```

### Step 2: Start Simulation

1. Type `startsim` in Serial Monitor
2. Press Enter
3. **Look for Firebase backup messages every 15 seconds:**

```
🔥 Firebase Backup [Attempt 1]:
   Path: /devices/[MAC]/readings/[timestamp]
   Temperature: 25.3°C
   Humidity: 65.2%
   Prediction: Sunny (4)
   Status: ✅ Upload successful
```

### Step 3: Check Firebase Database

1. Open Firebase Console: https://console.firebase.google.com/
2. Go to **Realtime Database**
3. Check for data at path: `/devices/[YOUR_MAC_ADDRESS]/`
4. Should see:
   - `info/` - Device metadata
   - `status/` - Online status
   - `readings/` - Sensor data with timestamps

---

## 🐛 Common Issues & Fixes

### Issue 1: "Firebase not initialized"
**Symptom:** No "STEP 3" messages in serial
**Fix:** Verify Firebase library installed
- Arduino IDE → Tools → Manage Libraries
- Search: "Firebase ESP Client"
- Install: "Firebase Arduino Client Library for ESP8266 and ESP32" by Mobizt

### Issue 2: "Connection failed"
**Symptom:** See "❌ Connection failed" in serial
**Possible causes:**
- WiFi not connected (check STEP 1)
- Wrong API key or database URL
- Firebase rules blocking access
- Network firewall blocking Firebase

**Fix:**
1. Verify WiFi connected in STEP 1
2. Check credentials match Firebase Console
3. Verify Firebase rules allow write access

### Issue 3: "Upload failed"
**Symptom:** See Firebase backup attempts but fails
**Fix:** Check Firebase Realtime Database Rules

**Recommended Rules:**
```json
{
  "rules": {
    "devices": {
      ".read": true,
      ".write": true,
      "$deviceId": {
        "readings": {
          ".indexOn": ["timestamp"]
        }
      }
    }
  }
}
```

### Issue 4: No Firebase messages at all
**Symptom:** System shows "READY" but no Firebase output
**Possible causes:**
- `FIREBASE_ENABLED` set to `false`
- Firebase code not compiling (check for errors)
- Serial output scrolled past

**Fix:**
1. Check `firebase_manager.h` line 54: `#define FIREBASE_ENABLED true`
2. Re-upload code to ESP32
3. Reset ESP32 and capture full boot log

---

## 📊 Expected Behavior

### Successful Firebase Operation:

**Boot sequence:**
1. WiFi connects ✅
2. ThingSpeak tests connection ✅
3. Firebase initializes ✅
4. System shows "READY" ✅

**During simulation (every 15 seconds):**
1. Generate sensor data
2. Upload to ThingSpeak
3. **Upload to Firebase** 🔥
4. Show backup status

**Firebase backup includes:**
- Device info (once on boot)
- Status updates (online/offline)
- Sensor readings (every 15 sec):
  - Temperature, Humidity, Pressure
  - Light, Gas (if available)
  - Weather prediction
  - Inference time
  - Timestamp

---

## 🔧 Quick Verification Commands

### Check if Firebase is enabled:
```cpp
// In firebase_manager.h line 54:
#define FIREBASE_ENABLED true  // Should be true
```

### Check backup interval:
```cpp
// In firebase_manager.h line 55:
#define BACKUP_INTERVAL 15000  // 15 seconds for testing
```

### Force immediate backup test:
After typing `startsim`, wait 15 seconds and look for:
```
🔥 Firebase Backup [Attempt X]:
```

---

## 📝 Next Steps

1. **Reset ESP32** and capture full boot sequence
2. **Look for "STEP 3: Firebase Backup"** messages
3. **Check connection status** (✅ or ❌)
4. **Type `startsim`** to begin simulation
5. **Wait 15 seconds** for first Firebase backup attempt
6. **Report back** with:
   - Full serial output from boot
   - Any error messages
   - Firebase backup status messages

---

## 🎯 What to Send Me

Please provide:
1. **Full Serial Monitor output** from ESP32 reset (include STEP 1, 2, 3)
2. **Firebase backup messages** after typing `startsim`
3. **Any error messages** (red text or ❌ symbols)
4. **Screenshot** of Firebase Console → Realtime Database

---

## ✅ Credentials Summary

| Item | Value | Status |
|------|-------|--------|
| API Key | AIzaSyDO5pK4EIi6N4n9PaZZ4GvS7wh52aPsiyU | ✅ Correct |
| Database URL | weather-prediction-syste-9d94d-default-rtdb.asia-southeast1.firebasedatabase.app | ✅ Correct |
| Project ID | weather-prediction-syste-9d94d | ✅ Correct |
| Storage Bucket | weather-prediction-syste-9d94d.firebasestorage.app | ✅ Correct |
| Messaging Sender ID | 560109147497 | ✅ Fixed |
| App ID | 1:560109147497:web:ef06ff5d4d28448ef8d9f8 | ✅ Fixed |
| Auth Domain | weather-prediction-syste-9d94d.firebaseapp.com | ✅ Added |
| Measurement ID | G-1JZRSJM8ZS | ✅ Added |

**ESP32 needs only:** API Key + Database URL (both correct ✅)  
**Frontend needs:** All fields (all correct now ✅)

---

**Status:** Ready for testing  
**Action Required:** Reset ESP32 and type `startsim` in Serial Monitor
