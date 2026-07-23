# Blynk Mobile App Dashboard Setup (Garden Moisture)

This guide mirrors the firmware pin map and matches the web dashboard setup.

## 1) Open template in mobile app

1. Install/open Blynk IoT app.
2. Sign in to the same account used for your template.
3. Open your template/device dashboard for Garden Moisture.
4. Enter Edit mode.

## 2) Datastream prerequisites

The mobile dashboard assumes these virtual datastreams already exist in your template:
- V0..V14 moisture percent
- V15 battery percent
- V16 sleep minutes
- V17 alert threshold
- V18 dry raw
- V19 wet raw
- V20 calibration mode
- V21 calibration zone
- V22 capture dry
- V23 capture wet
- V24 live raw ADC
- Optional: V25..V27 diagnostics

If needed, create datastreams first using [BLYNK_WEB_DASHBOARD_SETUP.md](BLYNK_WEB_DASHBOARD_SETUP.md).

## 3) Build mobile layout

Add widgets in this order.

### A) Top row (status and controls)

1. Gauge -> datastream V15
- Label: Battery
- Range: 0..100
- Unit: %

2. Slider -> datastream V17
- Label: Alert threshold
- Min: 0
- Max: 100
- Step: 1
- Send values on release: On

3. Step widget or Slider -> datastream V16
- Label: Sleep minutes
- Min: 5
- Max: 1440
- Step: 1

4. Styled Switch -> datastream V20
- Label: Calibration mode
- OFF value: 0
- ON value: 1

### B) Zones panel

Create 15 value displays for V0..V14:
- Labels: Zone 1 through Zone 15
- Unit: %
- Optional: use 3 columns to reduce scrolling

### C) Calibration panel

1. Slider -> datastream V21
- Label: Calibration zone
- Min: 1
- Max: 15
- Step: 1
- Send values on release: On

2. Value display -> datastream V24
- Label: Live raw ADC

3. Value display -> datastream V18
- Label: Dry raw

4. Value display -> datastream V19
- Label: Wet raw

5. Button -> datastream V22
- Label: Capture dry
- Mode: Push
- Press value: 1
- Release value: 0 (if the app asks)

6. Button -> datastream V23
- Label: Capture wet
- Mode: Push
- Press value: 1
- Release value: 0 (if the app asks)

### D) Optional diagnostics panel

- Value -> V25 (WiFi RSSI, dBm)
- Value -> V26 (Last report)
- Value -> V27 (Last error)

## 4) Mobile reliability notes

1. V21 must be 1..15. Do not use 0..14.
2. V22/V23 must be Push buttons, not Switch.
3. If controls seem ignored, the device may be sleeping. Toggle V20 on soon after device wakes.
4. During calibration sessions, keep the device online long enough to change V21 and use capture buttons.

## 5) Quick validation in app

1. Confirm device is online (green state).
2. Move V21 to another zone.
3. Verify V24 changes for the selected zone.
4. Tap Capture dry/wet and confirm V18/V19 update.
5. Turn V20 off and verify normal runtime behavior resumes.
