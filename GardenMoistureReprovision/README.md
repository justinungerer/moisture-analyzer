# GardenMoistureReprovision

Upload this sketch to clear the saved `blynk` provisioning data from the Nano ESP32.

## Use

1. Open `GardenMoistureReprovision` in Arduino IDE.
2. Upload it to the board.
3. Open Serial Monitor at `115200` and wait for `Blynk provisioning cleared.`
4. Re-open the main `GardenMoisture` sketch and upload it.
5. Provision the device again in the Blynk app.

This utility does not touch the garden calibration data stored in the `garden` preferences namespace.
