# Garden Moisture Boot Debugging Guide

This guide captures the proven process for diagnosing boot loops, reset reasons, and startup-stage crashes.

## 1) Core rule

Always isolate in layers:
1. board/USB stability
2. minimal serial alive loop
3. Wi-Fi-only
4. sensors init
5. Blynk/Edgent startup
6. sleep behavior

Do not change multiple layers at once.

## 2) Reset reason reference used in this project

- `Reset reason: 5` observed repeatedly during looping states.
- In this project flow, repeated `5` aligned with deep-sleep wake/reset cycling.

If you see recurring `5`, verify sleep logic first.

## 3) Proven diagnostic ladder

### Step A: Early halt sanity

Use an early infinite loop in `setup()` after serial starts:
- print `EARLY_HALT alive` every second

Interpretation:
- stable alive output: board/power path is fine
- still resetting: external/power/reset issue (not app logic)

### Step B: Wi-Fi-only mode

Skip sensors and Edgent, run only:
- provisioning config apply
- direct `WiFi.begin()` diagnostics
- periodic connected/disconnected print

If stable + IP appears:
- Wi-Fi credentials/radio path is good
- crash is elsewhere

### Step C: Sensors-only mode

Enable `sensorsBegin()` while still skipping Edgent.

If crash appears here:
- sensor init path is culprit

In this project, crash happened during direct-pin sensor init, which was fixed by moving to CD4051 mux-based reading.

### Step D: Full mode with sleep disabled

Re-enable full app path while keeping deep sleep off:
- prevents sleep from masking startup behavior
- confirms Blynk/Edgent and app logic without wake cycles

## 4) Known root cause found

Original `sensors.h` used a direct pin array that did not match actual hardware topology.

Hardware is:
- 2x CD4051 mux
- single ADC signal pin (`MUX_SIG_PIN`)
- channel select + inhibit pins

Fix implemented:
- replaced direct-pin reads with mux channel selection + `analogRead(MUX_SIG_PIN)`

## 5) Minimal debug markers that worked

Useful markers in `setup()`:
- `STEP 1: analogReadResolution`
- `STEP 2: calibrationLoad`
- `STEP 3: alertsLoadFromFlash`
- `STEP 4: set defaults`
- `STEP 5: sensorsBegin`
- `STEP 7: BlynkEdgent.begin`

When crash occurs, last printed marker identifies failing stage.

## 6) Why Serial looked blank earlier

Two compounding causes occurred:
- USB re-enumeration instability from BOOT/GPIO0 usage
- fast reset/sleep loop before monitor attached

Mitigation used:
- boot guard delay/countdown
- monitor open before reset
- non-BOOT button pin

## 7) Future crash triage checklist

1. Confirm firmware actually flashed (look for unique firmware tag in serial).
2. Check reset reason each boot.
3. Disable deep sleep.
4. Run early halt test.
5. Run Wi-Fi-only mode.
6. Run sensors-only mode.
7. Re-enable Edgent last.
8. Only after stable boot, re-enable deep sleep.

## 8) Safe temporary toggles

Use these temporary controls during debug:
- `ENABLE_DEEP_SLEEP false`
- optional runtime no-sleep guard in `enterDeepSleep()`
- mode flags for Wi-Fi-only / sensors-only

Remove these after stabilization.

## 9) Recovery after successful fix

After system is stable:
1. remove debug logs/guard loops
2. restore normal setup/loop flow
3. keep mux sensor implementation
4. keep non-BOOT button pin
5. re-enable deep sleep only after final validation

## 10) Last-known-good sequence

The sequence that succeeded:
1. early halt proved board stable
2. Wi-Fi-only proved LAN join worked
3. sensors-only exposed sensor init fault
4. mux-based sensor fix removed crash
5. full app mode restored with deep sleep disabled
6. device returned online
