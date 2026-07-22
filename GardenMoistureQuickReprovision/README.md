# GardenMoistureQuickReprovision

Minimal sketch to clear the saved `blynk` provisioning namespace as quickly as possible.

## Use

1. Open `GardenMoistureQuickReprovision` in Arduino IDE.
2. Upload it to the Nano ESP32.
3. Wait for the built-in LED to blink.
4. Re-open and upload the main `GardenMoisture` sketch.
5. Reprovision the device in the Blynk app.

This sketch only clears the `blynk` namespace. It does not erase the `garden` calibration/settings namespace.
