# Kaggle API Setup Guide

## Step 1: Create Kaggle Account
1. Go to https://www.kaggle.com/
2. Create an account or sign in

## Step 2: Generate API Token
1. Go to your Kaggle account settings: https://www.kaggle.com/account
2. Scroll down to the "API" section
3. Click "Create New API Token"
4. This will download a file called `kaggle.json`

## Step 3: Install API Token
### On Windows:
1. Create the directory: `C:\Users\{username}\.kaggle\`
2. Move the `kaggle.json` file to this directory
3. Or set environment variable: `set KAGGLE_CONFIG_DIR=C:\path\to\your\kaggle\folder`

### Alternative Method:
Set environment variables directly:
```bash
set KAGGLE_USERNAME=your_username
set KAGGLE_KEY=your_api_key
```

## Step 4: Test Connection
Run the following in your activated virtual environment:
```bash
kaggle datasets list
```

## Step 5: Download Weather Datasets
Example commands for weather-related datasets:
```bash
kaggle datasets download -d selfishgene/historical-hourly-weather-data
kaggle datasets download -d budincsevity/szeged-weather
kaggle datasets download -d zaraavagyan/weathercsv
```

## Security Notes:
- Never commit kaggle.json to version control
- Keep your API credentials secure
- Add kaggle.json to .gitignore

## For Team Members:
Each team member needs to set up their own Kaggle API credentials.
The datasets will be downloaded to the `data/external/` folder.
