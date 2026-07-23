# Garden Moisture Monitor Program Description

## Overview

Garden Moisture Monitor is an Arduino Nano ESP32 application that measures soil moisture across 15 zones, publishes the readings to Blynk, supports per-zone calibration, sends low-moisture alerts, and powers the board down between measurement cycles to save battery life.

The firmware is organized around a few clear responsibilities:

1. Read 15 capacitive soil sensors through two CD4051 analog multiplexers.
2. Convert raw ADC readings into moisture percentages using stored dry and wet calibration values.
3. Synchronize those values with a Blynk dashboard and accept dashboard commands.
4. Trigger low-moisture notifications at most once per zone per day.
5. Enter deep sleep after reporting so the system can run from a battery and solar supply.

## Hardware And Data Flow

The board uses one analog input for the soil sensor bank, one analog input for battery voltage, and several digital pins to control the multiplexers and sensor power switch. The sensor reading path is:

1. Wake the sensor bank if power switching is enabled.
2. Select one multiplexer channel for the requested zone.
3. Sample the analog value multiple times and average the result.
4. Convert the raw reading into a percentage using the zone calibration.
5. Send the result to Blynk and optionally compare it to the alert threshold.

Battery voltage is read separately and converted to a percentage using the configured voltage divider and battery limits.

## Main Program Flow

The program runs in a wake-report-sleep cycle.

### Boot and setup

When the board starts, `setup()` initializes serial logging, sets the ADC resolution, loads calibration data from flash, loads previous alert state, configures the sensor hardware, optionally applies home Wi-Fi provisioning, and starts Blynk Edgent.

After startup, it prints a short status summary showing the current sleep interval and alert threshold.

### Runtime loop

`loop()` keeps Blynk Edgent running and handles three background behaviors:

1. If calibration mode is active and the device is online, it periodically refreshes the live raw ADC value for the selected zone.
2. If the device has already reported for this wake cycle, it waits briefly for Blynk traffic and then enters deep sleep.
3. If deep sleep is disabled for debugging, it clears the one-shot report flag so the sleep logic does not spam logs.

## Program Functions

### `applyHomeWifiProvisioning()`

This function preloads Wi-Fi and optional Blynk token values into the Edgent config store. It is meant for a setup flow where the device should join a known home network immediately instead of waiting for access-point provisioning.

Behavior:

1. Checks whether the SSID and token placeholders have been replaced.
2. Copies the configured SSID, password, and token into the Edgent config store.
3. Marks the config as valid and saves it so the device can boot with those credentials.

### `zoneIndexFromPin()`

This helper converts the Blynk calibration zone value into a zero-based array index.

Behavior:

1. Clamps the incoming value to the valid range of 1 to 15.
2. Converts it to a zero-based index for internal arrays.

### `syncCalibrationToApp()`

This function pushes the current calibration state back to Blynk.

It updates:

1. The alert threshold.
2. Calibration mode on or off.
3. The currently selected zone.
4. The selected zone's dry raw value.
5. The selected zone's wet raw value.

This keeps the app aligned with the values stored in flash and in RAM.

### `captureCalibrationRaw(bool dryCapture)`

This function records the current raw ADC value as either the dry point or the wet point for the selected zone.

Behavior:

1. Powers the sensor bank on.
2. Reads the current raw value for the selected zone.
3. Powers the sensor bank back off.
4. Saves the new dry or wet calibration value for that zone.
5. Updates the Blynk widgets so the user can immediately see the captured value.

This is the primary calibration workflow used from the dashboard buttons.

### `checkLowMoistureAlerts(const int moisture[SENSOR_COUNT])`

This function checks all zones for moisture values below the configured alert threshold and sends a Blynk event when a zone qualifies.

Behavior:

1. Synchronizes the device clock using NTP and the configured timezone.
2. Skips alerting if time sync fails, which prevents repeated false notifications.
3. Scans each zone.
4. Ignores zones that are already above the threshold.
5. Sends the low-moisture event only if that zone has not already alerted today.
6. Stores the daily alert marker so the same zone does not spam notifications.

This is what enforces the one-alert-per-zone-per-day rule.

### `reportAll()`

This is the main reporting function. It gathers sensor values, formats them for the app, and publishes system status.

Behavior in normal mode:

1. Reads all raw sensor values.
2. Converts each raw value to a moisture percentage.
3. Writes V0 through V14 with the per-zone moisture percentages.
4. Runs low-moisture alert checks.
5. Reads battery percentage and writes it to Blynk.
6. Writes the current sleep interval.
7. Synchronizes calibration settings and selected zone state back to the app.

Behavior in calibration mode:

1. Reads all raw sensor values.
2. Writes raw ADC values instead of percentages to the moisture displays.
3. Updates the live raw reading for the selected zone.
4. Prints detailed calibration diagnostics to Serial.

This mode makes it easier to tune dry and wet reference values without guessing.

### `enterDeepSleep()`

This function ends the active wake cycle and puts the board into deep sleep.

Behavior:

1. Exits immediately if deep sleep is disabled for debugging.
2. Exits immediately if calibration mode is active, because calibration requires the board to stay awake.
3. Saves calibration settings so they survive reboot or sleep loss.
4. Configures the timer wakeup using the configured sleep interval.
5. Optionally enables a hardware wake button.
6. Starts deep sleep.

This is the primary power-saving feature of the program.

## Blynk Handlers

The sketch uses Blynk virtual write handlers to make the dashboard interactive.

### `BLYNK_WRITE(16)`

Updates the sleep interval in minutes. The accepted range is 5 to 1440 minutes.

### `BLYNK_WRITE(17)`

Updates the low-moisture alert threshold. The accepted range is 0 to 100 percent and the value is saved to flash.

### `BLYNK_WRITE(20)`

Turns calibration mode on or off and persists that state.

### `BLYNK_WRITE(21)`

Selects which zone is being calibrated and refreshes the calibration widgets in the app.

### `BLYNK_WRITE(18)` and `BLYNK_WRITE(19)`

Allow manual editing of the selected zone's dry and wet calibration values.

### `BLYNK_WRITE(22)` and `BLYNK_WRITE(23)`

Trigger calibration capture for dry and wet readings when the dashboard buttons are pressed.

### `BLYNK_CONNECTED()`

Runs after the device reconnects to Blynk. It stores the connection time, syncs calibration values to the app, sends a full report, and marks the wake cycle as already reported.

## Supporting Modules

The sketch depends on three supporting helper layers.

### Sensor helpers

The sensor helpers initialize the multiplexer pins, control sensor power, read raw moisture values, read battery voltage, and read all zones in sequence.

Important behavior:

1. Sensors are read one zone at a time through the CD4051 selection lines.
2. Each zone is sampled several times and averaged to reduce noise.
3. The multiplexer outputs are disabled between reads to reduce channel bleed.
4. Battery percentage is estimated from ADC voltage and the configured battery range.

### Calibration helpers

The calibration module stores and loads per-zone dry and wet ADC values and uses them to convert raw readings into percentages.

Important behavior:

1. Calibration data is stored in the ESP32 preferences flash namespace.
2. Defaults are used if no saved values exist.
3. The conversion logic handles both normal and inverted sensor wiring.
4. Percent values are constrained to the 0 to 100 range.

### Alert helpers

The alert module handles daily rate limiting for notifications.

Important behavior:

1. It stores the last alert day per zone in RTC memory and flash.
2. It syncs device time using NTP and the configured timezone.
3. It prevents duplicate alerts from being sent on the same local day.

## Operational Modes

The program effectively has three operating modes:

1. Normal monitoring mode, where it reports percentages and sleeps between wake cycles.
2. Calibration mode, where it stays awake and shows raw sensor values for tuning.
3. Debug mode with deep sleep disabled, which keeps the board active for troubleshooting.

## Blynk Pin Map

The Blynk dashboard is structured around the following data flow:

1. V0 to V14: moisture percentage for zones 1 to 15.
2. V15: battery percentage.
3. V16: sleep interval.
4. V17: alert threshold.
5. V18 and V19: selected zone dry and wet calibration values.
6. V20: calibration mode toggle.
7. V21: selected calibration zone.
8. V22 and V23: capture dry and wet buttons.
9. V24: live raw ADC value for the selected zone.

## Summary

This program is a low-power garden monitoring controller. It reads a bank of soil sensors, converts the readings into meaningful moisture values, exposes controls and diagnostics through Blynk, stores calibration and alert state in flash, and conserves battery by sleeping between measurement cycles.