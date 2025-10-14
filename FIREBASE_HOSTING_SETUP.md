# Firebase Hosting Setup Guide

## 🚀 Quick Setup Instructions

### Step 1: Install Firebase CLI

Open PowerShell and run:
```powershell
npm install -g firebase-tools
```

### Step 2: Login to Firebase

```powershell
firebase login
```

This will open your browser to authenticate with your Google account.

### Step 3: Initialize Firebase Project

Navigate to your project directory:
```powershell
cd "G:\CSE Weather Model"
```

Then link your Firebase project:
```powershell
firebase use --add
```

- Select your Firebase project from the list
- Give it an alias (e.g., "default" or "production")
- This will update the `.firebaserc` file

### Step 4: Deploy to Firebase Hosting

```powershell
firebase deploy --only hosting
```

## 📋 What Was Configured

### `firebase.json`
- **Public Directory**: Set to `frontend` folder (contains your HTML/CSS/JS)
- **Rewrites**: All routes redirect to `index.html` (for SPA functionality)
- **Caching**: 
  - JS/CSS/HTML: 1 hour cache
  - Images: 24 hour cache
- **Ignore**: Hidden files and node_modules excluded from deployment

### `.firebaserc`
- Contains your Firebase project ID
- You need to update `"your-firebase-project-id"` with your actual Firebase project ID

## 🔧 How to Find Your Firebase Project ID

1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Select your project
3. Click the gear icon → Project Settings
4. Copy the "Project ID" (not the project number)
5. Update `.firebaserc` with this ID

## 🌐 After Deployment

Your dashboard will be available at:
- **Primary URL**: `https://your-project-id.web.app`
- **Secondary URL**: `https://your-project-id.firebaseapp.com`

## 📊 Your Data Sources (No Changes Needed!)

Your frontend will continue to fetch data from:
- ✅ **ThingSpeak API**: `https://api.thingspeak.com/channels/3108323/feeds.json`
- ✅ **Firebase Realtime Database**: Your existing Firebase configuration
- ✅ **OpenWeatherMap API**: External weather data

All API calls work exactly the same from Firebase Hosting!

## 🔄 Updating Your Site

Whenever you make changes to your frontend:
```powershell
firebase deploy --only hosting
```

## 🎯 Custom Domain (Optional)

To add a custom domain like `weather.yourdomain.com`:
```powershell
firebase hosting:channel:deploy live
```

Then in Firebase Console:
1. Go to Hosting
2. Click "Add custom domain"
3. Follow the DNS configuration steps

## 🛡️ Security Notes

- Your API keys in `config.js` are exposed in the browser (this is normal for client-side apps)
- Consider using Firebase Security Rules to protect your database
- ThingSpeak Read API key is safe to expose (it's read-only)
- For production, consider environment-specific configs

## 📱 Testing Before Deployment

Test locally with Firebase emulator:
```powershell
firebase serve --only hosting
```

This will start a local server at `http://localhost:5000`

## 🚨 Troubleshooting

**Problem**: `firebase: command not found`
- Solution: Reinstall Firebase CLI or restart PowerShell

**Problem**: `Permission denied`
- Solution: Run `firebase login` again

**Problem**: `Error fetching data after deployment`
- Solution: Check browser console for CORS errors (shouldn't happen with Firebase Hosting)

## ✅ Checklist

- [ ] Install Firebase CLI
- [ ] Login to Firebase account
- [ ] Update `.firebaserc` with your project ID
- [ ] Run `firebase deploy --only hosting`
- [ ] Test the live URL
- [ ] Update documentation with live URL
- [ ] Remove or update `START_DASHBOARD.bat` (no longer needed)

---

**Your current setup**: All files ready for deployment!
**Estimated deployment time**: 1-2 minutes
**Cost**: FREE (Firebase Hosting free tier: 10 GB storage, 360 MB/day transfer)
