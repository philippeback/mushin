# deploy.ps1
# Deploys Mushin VST3 to the standard Windows VST3 location.
# Requires Administrator privileges.

$ErrorActionPreference = "Stop"

$vst3Source = "C:\Dev\github\philippeback\mushin\build2\Mushin_artefacts\Debug\VST3\Mushin.vst3"
$wv2Source = "C:\Dev\github\philippeback\mushin\build2\Mushin_artefacts\Debug\Standalone\WebView2Loader.dll"
$vst3Dest = "C:\Program Files\Common Files\VST3\Mushin.vst3"
$wv2Dest = Join-Path $vst3Dest "Contents\x86_64-win\WebView2Loader.dll"
$modInfoDest = Join-Path $vst3Dest "Contents\Resources\moduleinfo.json"

function Test-Admin {
    $currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    return $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Admin)) {
    Write-Warning "This script requires Administrator privileges. Attempting to elevate..."
    Start-Process powershell.exe -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs -Wait
    exit
}

Write-Host "Deploying VST3 from $vst3Source to $vst3Dest..."

if (-not (Test-Path $vst3Source)) {
    Write-Error "Source VST3 not found at $vst3Source. Did you build the project?"
    exit 1
}

# Ensure destination directory exists (parent)
$vst3Parent = Split-Path $vst3Dest
if (-not (Test-Path $vst3Parent)) {
    New-Item -ItemType Directory -Path $vst3Parent -Force
}

# Remove existing destination to ensure a clean overwrite
if (Test-Path $vst3Dest) {
    Write-Host "Removing existing VST3 at $vst3Dest..."
    try {
        Remove-Item -Path $vst3Dest -Recurse -Force -ErrorAction Stop
    } catch {
        Write-Error "Failed to remove existing VST3. It might be in use by another application (DAW). Please close your DAW and try again. Error: $($_.Exception.Message)"
        exit 1
    }
}

# Copy VST3 bundle
Write-Host "Copying VST3 bundle..."
Copy-Item -Path $vst3Source -Destination $vst3Dest -Recurse -Force

# Copy WebView2Loader.dll dependency if it exists
if (Test-Path $wv2Source) {
    Write-Host "Copying WebView2Loader.dll to $wv2Dest..."
    $wv2DestDir = Split-Path $wv2Dest
    if (-not (Test-Path $wv2DestDir)) {
        New-Item -ItemType Directory -Path $wv2DestDir -Force
    }
    Copy-Item -Path $wv2Source -Destination $wv2Dest -Force
}

# Sanitize moduleinfo.json (Remove trailing commas which can break some DAWs)
if (Test-Path $modInfoDest) {
    Write-Host "Sanitizing moduleinfo.json..."
    $content = Get-Content -Path $modInfoDest -Raw
    # Simple regex to remove trailing commas before closing braces/brackets
    # Note: This is a basic fix for the observed issue
    $content = $content -replace ',\s*([\]}])', '$1'
    Set-Content -Path $modInfoDest -Value $content


}

Write-Host "Deployment successful."
