$ErrorActionPreference = 'Stop'

function Invoke-ArduinoCli {
  param([Parameter(Mandatory = $true)][string[]]$CliArgs)
  & arduino-cli @CliArgs
  if ($LASTEXITCODE -ne 0) { throw "arduino-cli failed ($LASTEXITCODE): $($CliArgs -join ' ')" }
}

Invoke-ArduinoCli @('core', 'update-index')
Invoke-ArduinoCli @('core', 'install', 'esp32:esp32@2.0.17')
$libraries = @(
  'Adafruit BME280 Library@2.2.4',
  'Adafruit BusIO@1.16.2',
  'Adafruit GFX Library@1.11.11',
  'Adafruit SH110X@2.1.12',
  'Adafruit SSD1306@2.5.13',
  'Adafruit Unified Sensor@1.1.15',
  'ArduinoJson@6.21.5'
)
foreach ($library in $libraries) { Invoke-ArduinoCli @('lib', 'install', $library) }
Invoke-ArduinoCli @('compile', '--profile', 'wroom32', '.')
