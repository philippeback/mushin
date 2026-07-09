# rebuild_installer.ps1
# Automates the compilation of the Inno Setup installer and staging to the website downloads folder.
# Run this from the repository root: .\rebuild_installer.ps1

$ErrorActionPreference = "Stop"

# --- Define Paths ---
# Parse version from CMakeLists.txt
$cmakeFile = "CMakeLists.txt"
if (-not (Test-Path $cmakeFile)) {
    Write-Error "CMakeLists.txt not found at repository root!"
    exit 1
}
$cmakeContent = Get-Content $cmakeFile -Raw
if ($cmakeContent -match 'project\(Mushin\s+VERSION\s+([0-9\.]+)\)') {
    $version = $Matches[1]
    Write-Host "Parsed project version from CMakeLists.txt: $version" -ForegroundColor Green
} else {
    Write-Warning "Could not parse version from CMakeLists.txt. Defaulting to 1.0.0"
    $version = "1.0.0"
}

$issScript = "scripts\packaging\mushin_installer.iss"
$compiledInstaller = "build2\installer\Mushin_Windows_Installer_v$version.exe"
$siteDownloadsDir = "site\downloads"
$siteInstallerDest = Join-Path $siteDownloadsDir "Mushin_Windows_Installer_v$version.exe"

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
    }
    else {
        Write-Error "Inno Setup Compiler (ISCC.exe) was not found in your system's PATH or at '$standardPath'."
        Write-Host "Please install Inno Setup 6 or add it to your PATH." -ForegroundColor Yellow
        exit 1
    }
}

# 2. Compile Inno Setup Script
Write-Host "`n1/3 Compiling Inno Setup script: $issScript with version $version..." -ForegroundColor Blue
try {
    & $isccPath "/DAppVersion=$version" $issScript
}
catch {
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

# 4. Stage to Website Downloads Folder
Write-Host "`n2/3 Staging installer to public website downloads..." -ForegroundColor Blue
if (-not (Test-Path $siteDownloadsDir)) {
    Write-Host "Creating downloads directory: $siteDownloadsDir" -ForegroundColor DarkGray
    New-Item -ItemType Directory -Path $siteDownloadsDir -Force | Out-Null
}

try {
    Copy-Item -Path $compiledInstaller -Destination $siteInstallerDest -Force
    Write-Host "Copied to: $siteInstallerDest" -ForegroundColor Green

    # Create ZIP archive of the installer
    $zipDest = $siteInstallerDest -replace '\.exe$', '.zip'
    Write-Host "Creating ZIP archive at $zipDest..." -ForegroundColor Blue
    if (Test-Path $zipDest) {
        Remove-Item -Path $zipDest -Force
    }
    Compress-Archive -Path $siteInstallerDest -DestinationPath $zipDest -Force
    Write-Host "Created ZIP archive successfully." -ForegroundColor Green

    # Update site/index.html version references
    $indexHtmlFile = Join-Path $PSScriptRoot "site\index.html"
    if (Test-Path $indexHtmlFile) {
        Write-Host "Updating version references in $indexHtmlFile to v$version..." -ForegroundColor Blue
        $utf8NoBOM = New-Object System.Text.UTF8Encoding($false)
        $htmlContent = [System.IO.File]::ReadAllText($indexHtmlFile, $utf8NoBOM)
        $htmlContent = $htmlContent -replace 'Standalone v[0-9\.]+', "Standalone v$version"
        $htmlContent = $htmlContent -replace 'Mushin_Windows_Installer_v[0-9\.]+\.zip', "Mushin_Windows_Installer_v$version.zip"
        [System.IO.File]::WriteAllText($indexHtmlFile, $htmlContent, $utf8NoBOM)
        Write-Host "Updated index.html successfully!" -ForegroundColor Green
    }
}
catch {
    Write-Error "Failed to copy or package installer. Error: $($_.Exception.Message)"
    exit 1
}

# 4a. Compile and Stage PDF User Manual
Write-Host "`n2a/3 Compiling and staging PDF User Manual..." -ForegroundColor Blue
if (-not (Get-Command "node" -ErrorAction SilentlyContinue)) {
    Write-Warning "Node.js is not found in your system's PATH. Skipping PDF manual compilation."
}
else {
    try {
        # node scripts\compile_manual.js
    }
    catch {
        Write-Error "Failed to compile the PDF User Manual. Error: $($_.Exception.Message)"
        exit 1
    }
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
