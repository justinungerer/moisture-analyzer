Blynk Template Upload Instructions

This repository contains `blynk_template.json`, a structured mapping of widgets/pins for the Garden Moisture Monitor (15 zones).

Quick options to apply these widget settings:

1) Mobile app (recommended)
   - Open the Blynk IoT mobile app and select your device (or template) dashboard.
   - Tap the + (Add Widget) button and add widgets one-by-one following the `widgets` array in `blynk_template.json`.
   - Set the widget Pin to the V pin listed (eg. V0..V31) and configure ranges/labels as shown.

2) Blynk Console (web)
   - Sign in at https://blynk.cloud and open your Template or Device dashboard.
   - Edit the template or device and add widgets matching the entries in `blynk_template.json`.
   - The console UI does not always support direct JSON import of widgets; manual creation is the reliable method.

3) Bulk import (advanced / may not be supported)
   - Some versions of Blynk provide a template import/export JSON format. If your console supports importing templates, you can adapt `blynk_template.json` to match the console's schema and import.

Recommended widgets (summary):
- V0..V14: Value Display / Gauge (Zone 1..15 moisture %)
- V15: Battery % (Gauge)
- V16: Sleep interval (Value Display)
- V17: Alert threshold (Slider 0..100)
- V18..V19: Cal dry/wet (Value Display)
- V20: Calibration mode (Switch)
- V21: Calibration zone (Slider 1..15)
- V22..V23: Capture dry/wet (Button, Push)
- V24: Live raw ADC (Value Display)
- V25: Stay awake (Switch)
- V26: Report now (Button, Push)
- V27: Selected zone enabled (Switch)
- V28: Battery voltage (Value Display, Double, 2 decimals)
- V29: Low-battery threshold (Numeric Input)
- V30: Diagnostics (Text/Value Display)
- V31: Sensor power switching (Switch; 1 = toggle per read, 0 = force ON)

If you'd like, I can produce a Blynk Console-compatible template JSON (requires the console's exact schema/version). Tell me if you want me to attempt that and I'll pull the current schema.