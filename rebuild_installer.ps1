# rebuild_installer.ps1
# Automates the compilation of the Inno Setup installer and staging to the website downloads folder.
# Run this from the repository root: .\rebuild_installer.ps1

$ErrorActionPreference = "Stop"

# --- Define Paths ---
$issScript = "scripts\packaging\mushin_installer.iss"
$compiledInstaller = "build2\installer\Mushin_Windows_Installer_v1.0.0.exe"
$siteDownloadsDir = "site\downloads"
$siteInstallerDest = Join-Path $siteDownloadsDir "Mushin_Windows_Installer_v1.0.0.zip"

Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "             MUSHIN INSTALLER COMPILATION & SITE STAGING" -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan

# 1. Locate Inno Setup Compiler (ISCC.exe)
$isccPath = "ISCC.exe"
if (-not (Get-Command "ISCC.exe" -ErrorAction SilentlyContinue)) {
    $standardPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    if (Test-Path $standardPath) {
        $isccPath = $standardPath
        Write-Host "Found Inno Setup Compiler at: $isccPath" -ForegroundColor Green
    } else {
        Write-Error "Inno Setup Compiler (ISCC.exe) was not found in your system's PATH or at '$standardPath'."
        Write-Host "Please install Inno Setup 6 or add it to your PATH." -ForegroundColor Yellow
        exit 1
    }
}

# 2. Compile Inno Setup Script
Write-Host "`n1/3 Compiling Inno Setup script: $issScript..." -ForegroundColor Blue
try {
    & $isccPath $issScript
} catch {
    Write-Error "Failed to compile the Inno Setup installer. Error: $($_.Exception.Message)"
    exit 1
}

# 3. Verify Output
if (-not (Test-Path $compiledInstaller)) {
    Write-Error "Installer compilation finished, but output file was not found at: $compiledInstaller"
    exit 1
}
$installerSize = (Get-Item $compiledInstaller).Length
Write-Host "Successfully compiled installer: $compiledInstaller ($($installerSize / 1MB -as [int]) MB)" -ForegroundColor Green

# 4. Stage to Website Downloads Folder (Compress to ZIP to comply with Firebase Spark plan limits)
Write-Host "`n2/3 Staging installer as ZIP to public website downloads..." -ForegroundColor Blue
if (-not (Test-Path $siteDownloadsDir)) {
    Write-Host "Creating downloads directory: $siteDownloadsDir" -ForegroundColor DarkGray
    New-Item -ItemType Directory -Path $siteDownloadsDir -Force | Out-Null
}

# Clean up raw .exe in the downloads folder if it exists (Spark plan throws 400 errors for EXEs)
$siteExeFile = Join-Path $siteDownloadsDir "Mushin_Windows_Installer_v1.0.0.exe"
if (Test-Path $siteExeFile) {
    Remove-Item -Path $siteExeFile -Force
}

try {
    # Clean up old ZIP if it exists
    if (Test-Path $siteInstallerDest) {
        Remove-Item -Path $siteInstallerDest -Force
    }
    # Compress the exe directly to a zip inside site/downloads/
    Compress-Archive -Path $compiledInstaller -DestinationPath $siteInstallerDest -Force
    $zipSize = (Get-Item $siteInstallerDest).Length
    Write-Host "Successfully compressed and staged ZIP archive to: $siteInstallerDest ($($zipSize / 1MB -as [int]) MB)" -ForegroundColor Green
} catch {
    Write-Error "Failed to compress/copy installer to website downloads. Error: $($_.Exception.Message)"
    exit 1
}

# 5. Success Message and Next Steps
Write-Host "`n3/3 Process complete!" -ForegroundColor Blue
Write-Host "========================================================================" -ForegroundColor Green
Write-Host " Mushin Installer has been successfully rebuilt and staged on your site!" -ForegroundColor Green
Write-Host "========================================================================" -ForegroundColor Green
Write-Host "`nYou are now ready to deploy the website to production."
Write-Host "To deploy, run:" -ForegroundColor Yellow
Write-Host "  firebase deploy --only hosting" -ForegroundColor Cyan
Write-Host "`n========================================================================" -ForegroundColor Green
