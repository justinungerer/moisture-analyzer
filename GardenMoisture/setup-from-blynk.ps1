# Copies Blynk.Edgent support files into GardenMoisture/
# Run after installing the Blynk library in Arduino IDE.

$ErrorActionPreference = "Stop"

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$libraryRoots = @(
    "$env:USERPROFILE\Documents\Arduino\libraries\Blynk\examples\Blynk.Edgent\Edgent_ESP32",
    "$env:USERPROFILE\OneDrive\Documents\Arduino\libraries\Blynk\examples\Blynk.Edgent\Edgent_ESP32"
)

$sourceDir = $null
foreach ($root in $libraryRoots) {
    if (Test-Path $root) {
        $sourceDir = $root
        break
    }
}

if (-not $sourceDir) {
    Write-Host "Blynk library not found." -ForegroundColor Red
    Write-Host "Install it in Arduino IDE: Sketch -> Include Library -> Manage Libraries -> search 'Blynk'"
    exit 1
}

$files = @(
    "BlynkEdgent.h",
    "BlynkState.h",
    "ConfigMode.h",
    "ConfigStore.h",
    "Console.h",
    "Indicator.h",
    "OTA.h",
    "ResetButton.h"
)

Write-Host "Copying Edgent files from:" $sourceDir

foreach ($file in $files) {
    Copy-Item (Join-Path $sourceDir $file) (Join-Path $projectDir $file) -Force
    Write-Host "  OK $file"
}

Write-Host ""
Write-Host "Done. Open GardenMoisture.ino in Arduino IDE and upload." -ForegroundColor Green
