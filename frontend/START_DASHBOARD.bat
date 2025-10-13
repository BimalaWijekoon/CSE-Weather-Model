@echo off
REM Quick Start Script for Weather Dashboard
REM This script opens the dashboard in your default browser

echo ========================================
echo ESP32 Weather Dashboard - Quick Start
echo ========================================
echo.
echo Opening dashboard in your default browser...
echo.

REM Get the current directory
set "DASHBOARD_PATH=%~dp0index.html"

REM Open in default browser
start "" "%DASHBOARD_PATH%"

echo.
echo Dashboard opened successfully!
echo.
echo Features:
echo - Dashboard: Real-time sensor readings
echo - Graphs: Historical data visualization
echo - Predictions: ML weather predictions
echo - Activity: Live update log
echo - Device: ESP32-S3 system information
echo - WiFi: Network and connection status
echo.
echo The dashboard will auto-refresh every 5-10 seconds.
echo.
echo Press any key to exit...
pause >nul

