# Garden Moisture Monitor

Garden Moisture Monitor is an Arduino Nano ESP32 project for tracking soil moisture across 15 zones, reporting data to Blynk, supporting per-zone calibration, and conserving power with deep sleep between measurement cycles.

## What It Does

The firmware reads 15 capacitive moisture sensors through two CD4051 analog multiplexers, converts raw ADC values into moisture percentages, and publishes those values to a Blynk dashboard. It also measures battery level, supports manual calibration from the app, and sends low-moisture alerts with daily rate limiting.

## Core Behavior

1. Boot and load saved calibration, alert, and sleep settings.
2. Initialize the sensor hardware and Blynk connection.
3. Read all zones and calculate moisture percentages.
4. Push readings and status values to Blynk.
5. Send low-moisture alerts when a zone drops below the threshold.
6. Save any updated settings and enter deep sleep until the next wake cycle.

## Main Features

- 15 zone moisture monitoring.
- Per-zone dry and wet calibration stored in flash.
- Adjustable alert threshold and sleep interval from Blynk.
- Calibration mode that shows raw ADC values instead of percentages.
- Capture buttons for storing dry and wet reference values.
- One alert per zone per day using local time and NTP sync.
- Optional home-Wi-Fi-first provisioning for easier setup.
- Deep sleep support for battery-powered operation.

## Blynk Mapping

- V0 to V14: zone moisture percentages.
- V15: battery percentage.
- V16: sleep interval in minutes.
- V17: low-moisture alert threshold.
- V18 and V19: selected zone dry and wet calibration values.
- V20: calibration mode on or off.
- V21: selected calibration zone.
- V22 and V23: capture dry and wet buttons.
- V24: live raw ADC for the selected zone.

## Important Files

- [GardenMoisture.ino](GardenMoisture.ino) contains the main program flow and Blynk handlers.
- [config.h](config.h) defines pins, thresholds, and Blynk datastream IDs.
- [sensors.h](sensors.h) handles multiplexer control, raw sensor reads, and battery measurement.
- [calibration.h](calibration.h) stores calibration values and converts raw ADC readings to percentages.
- [alerts.h](alerts.h) manages daily alert tracking and time sync.
- [PROGRAM_DESCRIPTION.md](PROGRAM_DESCRIPTION.md) gives a more detailed function-by-function explanation.
- [BLYNK_WEB_DASHBOARD_SETUP.md](BLYNK_WEB_DASHBOARD_SETUP.md) explains the Blynk web dashboard layout.
- [BLYNK_MOBILE_DASHBOARD_SETUP.md](BLYNK_MOBILE_DASHBOARD_SETUP.md) explains the matching mobile app layout.

## Quick Notes

- Calibration mode should stay on only while you are tuning sensor values.
- Deep sleep is disabled automatically when calibration mode is active.
- If home Wi-Fi provisioning is enabled in `config.h`, the device will try to join the configured network on boot.

## Related Documentation

- [Program description](PROGRAM_DESCRIPTION.md)
- [Web dashboard setup](BLYNK_WEB_DASHBOARD_SETUP.md)
- [Mobile dashboard setup](BLYNK_MOBILE_DASHBOARD_SETUP.md)
- [Connection guide](CONNECTION_GUIDE.md)
- [Boot debugging guide](BOOT_DEBUGGING_GUIDE.md)