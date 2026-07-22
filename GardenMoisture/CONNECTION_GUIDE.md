# Garden Moisture Connection Guide

This guide captures the known-good connection workflow for this project.

## 1) Baseline assumptions

- Board: Arduino Nano ESP32
- Project: `GardenMoisture`
- Home Wi-Fi: 2.4 GHz
- `Settings.h` uses:
  - `BOARD_BUTTON_PIN 25` (not GPIO0/BOOT)
- `config.h` currently keeps deep sleep off while validating:
  - `ENABLE_DEEP_SLEEP false`

## 2) Why this setup matters

- Using GPIO0 (BOOT) for button/wake causes USB serial disconnect/re-enumeration while pressed.
- Moving to GPIO25 avoids serial instability during reset/debug.
- Keeping deep sleep disabled during setup/debug prevents wake/report/sleep cycles from hiding logs.

## 3) Preferred bring-up flow

1. Open `GardenMoisture` in Arduino IDE.
2. Confirm board is `Arduino Nano ESP32`.
3. Upload sketch.
4. Open Serial Monitor at `115200`.
5. Press `EN/RESET` once.

Expected behavior:
- sketch stays online
- joins home Wi-Fi
- Blynk can connect

## 4) Home-WiFi-first config in `config.h`

These values must be valid if using home-WiFi-first startup:

```cpp
#define HOME_WIFI_PROVISIONING true
#define HOME_WIFI_SSID "<your-ssid>"
#define HOME_WIFI_PASS "<your-password>"
#define HOME_BLYNK_TOKEN "<32-char token>"
```

Notes:
- If token is wrong or placeholder, Wi-Fi join may still work but cloud auth can fail.
- Router client list is the source of truth for LAN presence.

## 5) AP provisioning flow (fallback)

If home-WiFi-first is not desired, use AP provisioning:

1. Ensure AP mode is enabled in app logic (if temporarily forced during debug).
2. Scan for device AP name (starts with `Garden`).
3. Connect to AP.
4. Open:
   - `http://blynk.setup`
   - fallback: `http://192.168.4.1`
5. Enter home Wi-Fi credentials and token.

## 6) If upload disconnects USB

Symptoms:
- upload done, serial disappears
- COM port does not auto-reopen

Recovery sequence:
1. Close Serial Monitor.
2. Replug USB cable.
3. Double-tap reset to enter DFU mode if required.
4. Upload again.
5. Open monitor and press reset once.

## 7) Interpreting common upload outcomes

Success:
- `Opening DFU capable USB device...`
- `Download done.`
- `Done!`

Failure (board not in DFU):
- `No DFU capable USB device available`

Failure (USB transfer collapsed):
- `Error during download (LIBUSB_ERROR_PIPE)`

For DFU failures:
- disconnect external wiring during upload
- use direct USB port (no hub)
- try a known-good cable

## 8) Stable connection checklist

- `BOARD_BUTTON_PIN` is not 0
- deep sleep disabled while validating
- valid SSID/password/token in `config.h`
- serial shows normal startup (no boot loop)
- board appears in router clients
- Blynk online state eventually reaches running/heartbeat

## 9) After stable validation

When confirmed stable, optionally re-enable battery behavior:

```cpp
#define ENABLE_DEEP_SLEEP true
```

Then verify wake/report/sleep cycle with Blynk online expectations.
