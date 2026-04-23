# PowerShell script to build the Mushin project in build2
# Usage: powershell.exe -NoProfile -ExecutionPolicy Bypass -File build.ps1 [target]

param (
    [string]$Target = "Mushin_Standalone"
)

Write-Host "--- Starting build for target: $Target ---" -ForegroundColor Cyan

# Check if build2 exists
if (-not (Test-Path "build2")) {
    Write-Error "Error: 'build2' directory not found. Please ensure CMake has been configured."
    exit 1
}

# Run the build
cmake --build build2 --config Debug --target $Target

if ($LASTEXITCODE -eq 0) {
    Write-Host "--- Build Succeeded ---" -ForegroundColor Green
} else {
    Write-Host "--- Build Failed ---" -ForegroundColor Red
    exit $LASTEXITCODE
}
