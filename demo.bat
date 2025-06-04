@echo off
REM Smart Classroom Monitoring System Demo
echo ========================================
echo Smart Classroom Monitoring System Demo
echo ========================================
echo.

echo Starting server in background...
cd /d "d:\SekolahKuliah\AlprogGaming\finpro\build"
start /b "" finpro.exe server 8080

echo Waiting for server to start...
timeout /t 2 /nobreak >nul

echo.
echo Starting client simulation...
echo Sending sensor data to server...

REM Send some sample sensor data
echo Starting client...
start /b "" finpro.exe client 127.0.0.1 8080

echo.
echo Demo complete! Check the data files generated:
echo - sensor_data.bin (binary sensor data)
echo - anomaly_report.json (JSON anomaly report)
echo.
echo To view the data, run:
echo   finpro.exe
echo Then use commands like:
echo   query
echo   query anomalous
echo   query normal sort temp_desc
echo.
pause
