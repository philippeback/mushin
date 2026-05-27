# Script to convert threshold parameter values from 0..1 scale to -60..0 dB scale in presets
Get-ChildItem -Path "presets/*.xml" | ForEach-Object {
    $filePath = $_.FullName
    Write-Host "Processing $filePath..."
    $content = Get-Content -Path $filePath -Raw
    
    # Perform exact string replacements for threshold values
    # Old default/max value '1' (or '1.0') -> new '0' dB
    $content = $content -replace 'id="threshold" value="1"', 'id="threshold" value="0"'
    $content = $content -replace 'id="threshold" value="1.0"', 'id="threshold" value="0"'
    
    # Old value '0.5' (linear) -> new '-6.02' dB
    $content = $content -replace 'id="threshold" value="0.5"', 'id="threshold" value="-6.02"'
    
    # Old value '0.5399999618530273' (linear) -> new '-5.35' dB
    $content = $content -replace 'id="threshold" value="0.5399999618530273"', 'id="threshold" value="-5.35"'
    
    # Old value '0.6399999856948853' (linear) -> new '-3.88' dB
    $content = $content -replace 'id="threshold" value="0.6399999856948853"', 'id="threshold" value="-3.88"'
    
    Set-Content -Path $filePath -Value $content -NoNewline
}
Write-Host "Preset conversion completed successfully!"
