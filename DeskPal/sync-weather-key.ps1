$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$envPath = Join-Path $projectRoot '.env'
$headerPath = Join-Path $PSScriptRoot 'DeskPalSecrets.h'

if (-not (Test-Path -LiteralPath $envPath)) {
    throw 'Project-root .env file was not found.'
}

$entry = Get-Content -LiteralPath $envPath | Where-Object { $_ -match '^\s*WEATHER_API_KEY\s*=' } | Select-Object -First 1
if (-not $entry) {
    throw 'WEATHER_API_KEY was not found in the project-root .env file.'
}

$apiKey = ($entry -replace '^\s*WEATHER_API_KEY\s*=\s*', '').Trim()
if ($apiKey.Length -ge 2 -and (($apiKey.StartsWith('"') -and $apiKey.EndsWith('"')) -or ($apiKey.StartsWith("'") -and $apiKey.EndsWith("'")))) {
    $apiKey = $apiKey.Substring(1, $apiKey.Length - 2)
}
if ([string]::IsNullOrWhiteSpace($apiKey)) {
    throw 'WEATHER_API_KEY is empty.'
}

$escapedKey = $apiKey.Replace('\', '\\').Replace('"', '\"').Replace("`r", '\r').Replace("`n", '\n')
$header = "#pragma once`r`n#define DESKPAL_WEATHER_API_KEY `"$escapedKey`"`r`n"
[System.IO.File]::WriteAllText($headerPath, $header, [System.Text.UTF8Encoding]::new($false))
Write-Host 'Weather API key synced into the ignored firmware header. The key was not printed.'
