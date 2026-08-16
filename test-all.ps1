param(
    [switch]$SkipBuild,
    [switch]$SkipFirmwareBuild
)

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot

function Invoke-DomTestStep {
    param(
        [Parameter(Mandatory)] [string]$Name,
        [Parameter(Mandatory)] [scriptblock]$Action
    )

    Write-Host "`n== $Name ==" -ForegroundColor Cyan
    & $Action
    if ($LASTEXITCODE -ne 0) {
        throw "$Name failed with exit code $LASTEXITCODE"
    }
}

$VenvPython = Join-Path $ProjectRoot "backend-python\.venv\Scripts\python.exe"
$Python = if (Test-Path -LiteralPath $VenvPython) { $VenvPython } else { "python" }

Invoke-DomTestStep "Python AI Gateway tests" {
    Push-Location (Join-Path $ProjectRoot "backend-python")
    try { & $Python -m unittest discover -s tests -v } finally { Pop-Location }
}

Invoke-DomTestStep "ESP32-S3 firmware contract tests" {
    Push-Location (Join-Path $ProjectRoot "firmware")
    try { & $Python -m unittest discover -s tests -v } finally { Pop-Location }
}

Invoke-DomTestStep "Go Core Backend tests" {
    Push-Location (Join-Path $ProjectRoot "backend-go")
    try {
        & go test ./...
        if ($LASTEXITCODE -eq 0) { & go vet ./... }
    } finally { Pop-Location }
}

Invoke-DomTestStep "Next.js Dashboard tests" {
    Push-Location (Join-Path $ProjectRoot "dashboard-next")
    try {
        & npm test
        if ($LASTEXITCODE -eq 0) { & npm run lint }
    } finally { Pop-Location }
}

if (-not $SkipBuild) {
    Invoke-DomTestStep "Go Core Backend build" {
        Push-Location (Join-Path $ProjectRoot "backend-go")
        try { & go build ./... } finally { Pop-Location }
    }

    Invoke-DomTestStep "Next.js production build" {
        Push-Location (Join-Path $ProjectRoot "dashboard-next")
        try { & npm run build } finally { Pop-Location }
    }
}

if (-not $SkipBuild -and -not $SkipFirmwareBuild) {
    $IdfPython = "C:\Espressif\python_env\idf5.3_py3.11_env\Scripts\python.exe"
    $IdfScript = "C:\Espressif\frameworks\esp-idf-v5.3.1\tools\idf.py"
    if ((Test-Path -LiteralPath $IdfPython) -and (Test-Path -LiteralPath $IdfScript)) {
        Invoke-DomTestStep "ESP32-S3 firmware build" {
            Push-Location (Join-Path $ProjectRoot "firmware")
            try {
                $env:IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.3.1"
                $env:IDF_PYTHON_ENV_PATH = "C:\Espressif\python_env\idf5.3_py3.11_env"
                $IdfTools = @(
                    "C:\Espressif\python_env\idf5.3_py3.11_env\Scripts",
                    "C:\Espressif\tools\cmake\3.24.0\bin",
                    "C:\Espressif\tools\ninja\1.11.1",
                    "C:\Espressif\tools\xtensa-esp-elf\esp-13.2.0_20240530\xtensa-esp-elf\bin"
                ) -join ";"
                $env:PATH = "$IdfTools;$env:PATH"
                & $IdfPython $IdfScript build
            } finally { Pop-Location }
        }
    } else {
        Write-Warning "ESP-IDF 5.3.1 was not found; firmware build skipped. Contract tests still passed."
    }
}

Write-Host "`nAll DomOS tests completed successfully." -ForegroundColor Green
