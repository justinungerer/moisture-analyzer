# Local Secrets Example

Copy this to `LOCAL_SECRETS.md` (which is gitignored) and fill with your private values.

## `GardenMoisture/config.h` values

```cpp
#define HOME_WIFI_SSID "YOUR_WIFI_SSID"
#define HOME_WIFI_PASS "YOUR_WIFI_PASSWORD"
#define HOME_BLYNK_TOKEN "YOUR_32_CHAR_BLYNK_TOKEN"
```

## Deep sleep mode

Debug mode:

```cpp
#define ENABLE_DEEP_SLEEP false
```

Battery/runtime mode:

```cpp
#define ENABLE_DEEP_SLEEP true
```

## Pre-push check

```powershell
git diff -- GardenMoisture/config.h
git status --short
```

Make sure real credentials are not staged.
