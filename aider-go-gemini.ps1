# aider-go.ps1
# This script starts aider with the Gemini model.
# It will prompt for your API key if it's not already set in the environment.

if (-not $env:GEMINI_API_KEY) {
    Write-Host "GEMINI_API_KEY not found in environment." -ForegroundColor Yellow
    $key = Read-Host "Please enter your Gemini API Key"
    if ($key) {
        $env:GEMINI_API_KEY = $key
        Write-Host "API Key set for this session." -ForegroundColor Green
    }
    else {
        Write-Host "No key provided. Aider may fail to start." -ForegroundColor Red
    }
}

# Start aider using the Pro 1.5 model
aider --model gemini/gemini-1.5-pro-latest

