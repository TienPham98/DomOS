$env:IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.3.1"
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\python_env\idf5.3_py3.11_env"
$env:PATH = "C:\Espressif\python_env\idf5.3_py3.11_env\Scripts;C:\Espressif\tools\cmake\3.24.0\bin;C:\Espressif\tools\ninja\1.11.1;C:\Espressif\tools\xtensa-esp-elf\esp-13.2.0_20240530\xtensa-esp-elf\bin;" + $env:PATH

$python = "C:\Espressif\python_env\idf5.3_py3.11_env\Scripts\python.exe"
$idf = "C:\Espressif\frameworks\esp-idf-v5.3.1\tools\idf.py"

Set-Location "d:\Work space\DomOS\firmware"
& $python $idf -p COM5 flash

