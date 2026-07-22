# GardenMoistureWiFiOnly

Minimal sketch to verify the Nano ESP32 can join your Wi-Fi without Blynk.

## Before upload

Edit these two lines in `GardenMoistureWiFiOnly.ino`:

```cpp
const char* kSsid = "YOUR_WIFI_SSID";
const char* kPassword = "YOUR_WIFI_PASSWORD";
```

Use your 2.4 GHz Wi-Fi network credentials.

## What it does

- connects to Wi-Fi only
- prints connection status to Serial Monitor at `115200`
- prints the assigned IP address
- retries automatically if disconnected

## Success

You should see:

- `WiFi connected`
- `IP: ...`
