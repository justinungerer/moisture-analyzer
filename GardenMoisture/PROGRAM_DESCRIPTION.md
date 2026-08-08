# Garden Moisture Monitor Program Description

Firmware version: 1.3.0

## Overview

Garden Moisture Monitor is an Arduino Nano ESP32 application that measures soil
moisture across 15 zones, publishes the readings to Blynk, supports per-zone
calibration and enable/disable, sends low-moisture and low-battery alerts, and
powers the board down between measurement cycles to run from battery and solar.

The firmware is organized around a few clear responsibilities:

1. Read 15 capacitive soil sensors through one CD74HC4067 analog multiplexer, with
   trimmed-mean filtering to reject noise.
2. Convert raw ADC readings into moisture percentages using per-zone dry/wet
   calibration values.
3. Keep the Blynk dashboard as the source of truth for settings, pulling them
   down on connect and pushing telemetry up.
4. Trigger low-moisture (per zone) and low-battery notifications at most once
   per day.
5. Stay online long enough for dashboard commands to land, then deep-sleep —
   with a hard wall-clock guard that forces sleep even if the cloud never
   connects, so a failed connection can never drain the battery.

## Sync Model (why the dashboard behaves correctly)

The device is asleep most of the time, so Blynk stores widget values on the
server. To avoid the classic bug where the device pushes its own defaults up
and overwrites the user's settings, pins are split by ownership:

- **Global settings (app-owned):** V16 sleep, V17 alert threshold, V20 cal
   mode, V21 zone select, V25 stay-awake, V29 low-battery threshold, V31 sensor
   power switching. On connect these are pulled *down* with
   `Blynk.syncVirtual()`. The device never writes them up during normal
   operation.
- **Telemetry (device-owned):** V0–V15, V24, V28, V30. Always written *up*.
- **Per-zone values (device-owned, edited live):** V18/V19 raw calibration and
  V27 enable. One widget maps to 15 backing values depending on the selected
  zone, so a sync-down would be ambiguous. These are pushed up for the selected
  zone and edited live while the device is online (calibration or stay-awake).
- **One-time seeding:** on the first successful connect ever (tracked by a
  persisted `provisioned` flag), the device seeds the setting widgets with its
  stored values so they show sensible initial values, then never clobbers them
  again.

## Hardware And Data Flow

The board uses one analog input for the soil-sensor bank (through the mux),
one analog input for battery voltage, and several digital pins to drive the
multiplexers and the sensor power switch. The sensor reading path is:

1. Wake the sensor bank if power switching is enabled.
2. Select one multiplexer channel for the requested zone.
3. Take multiple ADC samples, sort them, drop the extreme high/low samples, and
   average the remainder (trimmed mean) to reject spikes and dropouts.
4. Convert the filtered raw reading into a percentage using the zone
   calibration.
5. Write the result to Blynk and, in normal mode, compare it to the alert
   threshold.

Battery voltage is read as an averaged, attenuation-corrected ADC value and
converted to a state-of-charge percentage using a piecewise Li-ion discharge
curve (more accurate than a straight voltage-to-percent line).

## Main Program Flow

The program runs in a wake–report–(stay online)–sleep cycle.

### Boot and setup

`setup()` initializes serial logging, loads persistent settings from flash
(calibration, per-zone enable, sleep interval, low-battery threshold, selected
zone), loads previous alert state, configures the sensor hardware (ADC
resolution and attenuation, mux pins, power switch), optionally applies home
Wi-Fi provisioning, starts Blynk Edgent, and records the boot time used by the
sleep guard. It then prints the wake reason, sleep interval, and alert
threshold.

### Runtime loop

`loop()` keeps Blynk Edgent running and handles:

1. Live raw refresh for the selected zone while calibration mode is active.
2. Periodic re-reporting (every `LIVE_REPORT_INTERVAL_MS`) while the device is
   deliberately staying awake (stay-awake switch, calibration mode, or a debug
   build with deep sleep disabled).
3. Sleep servicing: sleep once the post-connect awake window elapses, or force
   sleep once the absolute max-awake cap elapses regardless of connection.

## Program Functions

### `applyHomeWifiProvisioning()`

Preloads Wi-Fi and an optional Blynk token into the Edgent config store so the
device can join a known network immediately instead of waiting in access-point
provisioning mode. It checks that the placeholders were replaced, copies the
credentials, marks the config valid, and saves it.

### `zoneIndexFromPin()`

Converts the Blynk 1–15 zone value into a zero-based array index, clamping to
the valid range.

### `shouldStayAwake()`

Returns true when the device must not deep-sleep: the stay-awake switch is on,
calibration mode is active, or deep sleep is disabled in the build.

### `pushZoneDisplay()`

Pushes the selected zone's device-owned state to the app: dry raw (V18), wet raw
(V19), and enabled flag (V27).

### `seedSettingsToApp()` / `syncSettingsFromApp()`

`seedSettingsToApp()` runs once on first connect and writes the device's setting
values up so the widgets initialize correctly. `syncSettingsFromApp()` runs on
every later connect and pulls the global settings down with `Blynk.syncVirtual()`
so the app remains the source of truth.

### `captureCalibrationRaw(bool dryCapture)`

Records the current raw ADC value as the dry or wet point for the selected zone:
powers the sensor bank on, reads the zone, powers it off, saves the value, and
updates the app widgets so the captured value is visible immediately.

### `checkLowMoistureAlerts(const int moisture[SENSOR_COUNT])`

Synchronizes device time, then scans each zone. It skips disabled zones and
zones above the threshold, and sends a `low_moisture` event only if that zone
has not already alerted today, recording the daily marker. If time sync fails it
skips alerting to avoid false or duplicate notifications.

### `checkLowBatteryAlert(int battPct)`

Sends a `low_battery` event at most once per day when the battery percentage is
at or below the configured low-battery threshold.

### `publishDiagnostics(int battPct)`

Writes a human-readable status string to V30 containing uptime, Wi-Fi RSSI, free
heap, battery percentage, wake reason, and firmware version.

### `reportAll()`

The main reporting function:

1. Reads and filters all raw sensor values and converts them to percentages.
2. Writes V0–V14 with per-zone moisture percentages (always percentages, even in
   calibration mode).
3. Writes battery percentage (V15) and battery voltage (V28).
4. Pushes the selected zone's calibration/enable display and live raw (V24).
5. Runs low-moisture alerts (normal mode only) and the low-battery alert.
6. Publishes the diagnostics string and logs a detailed report to Serial.

### `enterDeepSleep()`

Ends the active wake cycle and enters deep sleep: it returns immediately in
debug builds, optionally enables the hardware wake button, arms the timer wakeup
using the stored sleep interval, and starts deep sleep. Settings are already
persisted on change, so no flash write is needed here.

### `serviceSleep()`

Decides when to sleep. It never sleeps while `shouldStayAwake()` is true.
Otherwise it sleeps once the post-connect awake window (`AWAKE_WINDOW_MS`) has
elapsed, and it force-sleeps once the absolute cap (`MAX_AWAKE_MS`) elapses even
if the cloud never connected — the critical battery-drain guard.

## Blynk Handlers

| Handler | Pin | Purpose |
| --- | --- | --- |
| `BLYNK_WRITE(16)` | V16 | Sleep interval (minutes), clamped 5–1440, persisted |
| `BLYNK_WRITE(17)` | V17 | Low-moisture threshold %, persisted |
| `BLYNK_WRITE(29)` | V29 | Low-battery threshold %, persisted |
| `BLYNK_WRITE(20)` | V20 | Calibration mode on/off, persisted |
| `BLYNK_WRITE(25)` | V25 | Stay-awake switch (session) |
| `BLYNK_WRITE(31)` | V31 | Sensor power switching on/off, persisted |
| `BLYNK_WRITE(21)` | V21 | Selected calibration zone, persisted |
| `BLYNK_WRITE(27)` | V27 | Enable/disable the selected zone, persisted |
| `BLYNK_WRITE(18)` / `(19)` | V18/V19 | Manual dry/wet raw edit for selected zone |
| `BLYNK_WRITE(22)` / `(23)` | V22/V23 | Capture dry/wet (buttons auto-reset to 0) |
| `BLYNK_WRITE(26)` | V26 | Report-now button (auto-resets to 0) |
| `BLYNK_CONNECTED()` | — | Seed or sync settings, report, start the awake window |

Note: `BLYNK_WRITE` handlers use numeric pin literals (required so the Blynk
macro pastes into the correct `BlynkWidgetWrite<N>` handler); the numbers are
kept in sync with the `VPIN_*` names in `config.h`.

## Supporting Modules

### Sensor helpers (`sensors.h`)

Initialize the multiplexer pins, set ADC resolution and 11 dB attenuation,
control sensor power, and read zones one at a time. Each zone is sampled
`SENSOR_SAMPLES` times; the highest and lowest `SENSOR_DISCARD` samples are
dropped and the rest averaged (trimmed mean). Mux outputs are disabled between
reads to reduce channel bleed. Battery voltage is averaged over
one settled ADC read per report (with an initial throwaway conversion) and
converted through a piecewise Li-ion curve.

### Calibration / settings helpers (`calibration.h`)

Store and load all persistent state in the ESP32 preferences flash namespace:
per-zone dry/wet raw values, per-zone enable flags, alert and low-battery
thresholds, sleep interval, selected zone, calibration mode, and the
`provisioned` flag. Defaults are used when no saved value exists. Small helpers
persist individual scalars without rewriting the whole struct (to spare flash).
The conversion logic handles both normal and inverted sensor wiring and clamps
to 0–100%.

### Alert helpers (`alerts.h`)

Handle daily rate limiting for both moisture and battery notifications. The last
alert day is stored per zone (and once for battery) in RTC memory and flash.
Device time is synced with NTP and the configured timezone, and duplicate alerts
on the same local day are suppressed.

## Operational Modes

1. **Normal monitoring** — reports percentages, stays online for the awake
   window, then deep-sleeps.
2. **Stay-awake** — the V25 switch keeps the device online and re-reporting for
   live monitoring and configuration.
3. **Calibration** — the device stays awake, streams live raw ADC for the
   selected zone, and the moisture gauges continue to show percentages.
4. **Debug** — deep sleep disabled in the build; behaves like stay-awake for
   troubleshooting.

## Blynk Pin Map

| Pin | Direction | Meaning |
| --- | --- | --- |
| V0–V14 | device → app | Moisture % for zones 1–15 |
| V15 | device → app | Battery % |
| V16 | app → device | Sleep interval (minutes) |
| V17 | app → device | Low-moisture threshold % |
| V18 / V19 | both | Selected zone dry / wet raw |
| V20 | app → device | Calibration mode |
| V21 | app → device | Selected calibration zone (1–15) |
| V22 / V23 | app → device | Capture dry / wet buttons |
| V24 | device → app | Live raw ADC for selected zone |
| V25 | app → device | Stay-awake switch |
| V26 | app → device | Report-now button |
| V27 | both | Enable/disable selected zone |
| V28 | device → app | Battery voltage (V) |
| V29 | app → device | Low-battery threshold % |
| V30 | device → app | Diagnostics string |
| V31 | app → device | Sensor power switching (1=toggled, 0=forced ON) |

### Required Blynk Console setup

Create datastreams V25 (int 0/1), V26 (int 0/1), V27 (int 0/1), V28 (double, V),
V29 (int 0–100), V30 (string), and V31 (int 0/1), and create event codes
`low_moisture` and `low_battery`. V0–V24 are unchanged from earlier versions.

## Key Configuration Constants (`config.h`)

| Constant | Default | Purpose |
| --- | --- | --- |
| `DEFAULT_SLEEP_MINUTES` | 30 | Wake interval |
| `AWAKE_WINDOW_MS` | 45000 | Time online after connect to receive commands |
| `MAX_AWAKE_MS` | 120000 | Absolute cap; forces sleep even if never connected |
| `LIVE_REPORT_INTERVAL_MS` | 10000 | Re-report cadence while staying awake |
| `SENSOR_SAMPLES` / `SENSOR_DISCARD` | 12 / 2 | Trimmed-mean filtering |
| `BATTERY_ADC_SETTLE_MS` | 40 | Delay before single battery ADC read |
| `BATTERY_SMOOTHING_ENABLED` / `BATTERY_SMOOTHING_ALPHA_PCT` | true / 25 | Cross-report battery smoothing |
| `DEFAULT_ALERT_THRESHOLD` | 25 | Low-moisture % |
| `DEFAULT_LOW_BATT_THRESHOLD` | 15 | Low-battery % |

## Summary

This program is a low-power garden monitoring controller. It reads a bank of
soil sensors with noise filtering, converts the readings into meaningful
moisture values, keeps the Blynk dashboard authoritative for settings while
publishing rich telemetry and diagnostics, sends daily-limited moisture and
battery alerts, stores all state in flash, and conserves battery by staying
online only as long as needed — with a hard guard that guarantees it always
returns to deep sleep.
