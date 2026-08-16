@echo off
set "IDF_PATH=C:\Espressif\frameworks\esp-idf-v5.3.1"
set "IDF_PYTHON_ENV_PATH=C:\Espressif\python_env\idf5.3_py3.11_env"
set "PATH=C:\Espressif\python_env\idf5.3_py3.11_env\Scripts;C:\Espressif\tools\cmake\3.24.0\bin;C:\Espressif\tools\ninja\1.11.1;C:\Espressif\tools\xtensa-esp-elf\esp-13.2.0_20240530\xtensa-esp-elf\bin;%PATH%"

cd /d "%~dp0"
echo [1/3] Cleaning old build...
if exist build ( rmdir /s /q build )

echo [2/3] Building firmware...
"C:\Espressif\python_env\idf5.3_py3.11_env\Scripts\python.exe" "C:\Espressif\frameworks\esp-idf-v5.3.1\tools\idf.py" build
if errorlevel 1 exit /b %errorlevel%

echo [3/3] Build complete. Use flash.bat to flash to ESP32.
