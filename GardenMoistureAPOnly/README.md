# GardenMoistureAPOnly

Minimal AP-only sketch to verify the ESP32 is visible in Wi-Fi scan results.

## Steps

1. Upload this sketch.
2. Scan for Wi-Fi from your phone/laptop.
3. Look for `Garden-Setup-TEST`.
4. Connect to it (open network).
5. Open Serial Monitor at 115200 to see station count.

If this AP is visible, your radio is fine and the issue is in the Edgent/provisioning flow.
If this AP is not visible, the problem is below the app layer (upload/runtime/board state).
