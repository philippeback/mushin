$logFile = "C:\Dev\github\philippeback\mushin\deploy_log.txt"
"Starting deployment script..." | Out-File $logFile

$src = "C:\Dev\github\philippeback\mushin\build2\Mushin_artefacts\Release\VST3\Mushin.vst3"
$dest = "C:\Program Files\Common Files\VST3\Mushin.vst3"

$dllSrc = "C:\Dev\github\philippeback\mushin\build2\Mushin_artefacts\Release\Standalone\WebView2Loader.dll"

try {
    # Ensure WebView2Loader.dll is in the local VST3 bundle before copying
    if (Test-Path $dllSrc) {
        $localDllDest = Join-Path $src "Contents\x86_64-win"
        "Copying WebView2Loader.dll to local VST3 bundle..." | Out-File $logFile -Append
        if (-not (Test-Path $localDllDest)) {
            New-Item -ItemType Directory -Path $localDllDest -Force | Out-Null
        }
        Copy-Item -Path $dllSrc -Destination $localDllDest -Force
    } else {
        "Warning: WebView2Loader.dll not found in Release\Standalone!" | Out-File $logFile -Append
    }

    if (Test-Path $dest) {
        "Target exists, removing..." | Out-File $logFile -Append
        Remove-Item -Path $dest -Recurse -Force
    }
    
    "Copying from $src to C:\Program Files\Common Files\VST3\..." | Out-File $logFile -Append
    Copy-Item -Path $src -Destination "C:\Program Files\Common Files\VST3" -Recurse -Force
    
    $deployedFile = "C:\Program Files\Common Files\VST3\Mushin.vst3\Contents\x86_64-win\Mushin.vst3"
    if (Test-Path $deployedFile) {
        $date = (Get-Item $deployedFile).LastWriteTime.ToString("yyyy-MM-dd HH:mm:ss")
        "Successfully deployed VST3! New file write time: $date" | Out-File $logFile -Append
    } else {
        "Copy finished, but target file not found!" | Out-File $logFile -Append
    }
}
catch {
    "Error occurred: $($_.Exception.Message)" | Out-File $logFile -Append
}
