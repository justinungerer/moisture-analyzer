# Blynk Web Dashboard Setup (Garden Moisture)

This guide builds a web dashboard that matches the firmware pin map in config.h.

## 1) Create or open template

1. Sign in to Blynk Console: https://blynk.cloud
2. Open your template with ID TMPL2c6XdcWxv (or your current template).
3. In Template -> Info, verify hardware type is ESP32.

## 2) Create datastreams

Create virtual datastreams exactly as listed:

| Pin | Name | Type | Min | Max | Direction | Unit |
|---|---|---:|---:|---:|---|---|
| V0 | Zone 1 | Integer | 0 | 100 | Device -> Cloud | % |
| V1 | Zone 2 | Integer | 0 | 100 | Device -> Cloud | % |
| V2 | Zone 3 | Integer | 0 | 100 | Device -> Cloud | % |
| V3 | Zone 4 | Integer | 0 | 100 | Device -> Cloud | % |
| V4 | Zone 5 | Integer | 0 | 100 | Device -> Cloud | % |
| V5 | Zone 6 | Integer | 0 | 100 | Device -> Cloud | % |
| V6 | Zone 7 | Integer | 0 | 100 | Device -> Cloud | % |
| V7 | Zone 8 | Integer | 0 | 100 | Device -> Cloud | % |
| V8 | Zone 9 | Integer | 0 | 100 | Device -> Cloud | % |
| V9 | Zone 10 | Integer | 0 | 100 | Device -> Cloud | % |
| V10 | Zone 11 | Integer | 0 | 100 | Device -> Cloud | % |
| V11 | Zone 12 | Integer | 0 | 100 | Device -> Cloud | % |
| V12 | Zone 13 | Integer | 0 | 100 | Device -> Cloud | % |
| V13 | Zone 14 | Integer | 0 | 100 | Device -> Cloud | % |
| V14 | Zone 15 | Integer | 0 | 100 | Device -> Cloud | % |
| V15 | Battery | Integer | 0 | 100 | Device -> Cloud | % |
| V16 | Sleep minutes | Integer | 5 | 1440 | Cloud -> Device | min |
| V17 | Alert threshold | Integer | 0 | 100 | Cloud -> Device | % |
| V18 | Calibration dry raw | Integer | 0 | 4095 | Cloud <-> Device | |
| V19 | Calibration wet raw | Integer | 0 | 4095 | Cloud <-> Device | |
| V20 | Calibration mode | Integer | 0 | 1 | Cloud <-> Device | |
| V21 | Calibration zone | Integer | 1 | 15 | Cloud <-> Device | |
| V22 | Capture dry | Integer | 0 | 1 | Cloud -> Device | |
| V23 | Capture wet | Integer | 0 | 1 | Cloud -> Device | |
| V24 | Live raw | Integer | 0 | 4095 | Device -> Cloud | |
| V25 | Stay awake | Integer | 0 | 1 | Cloud -> Device | |
| V26 | Report now | Integer | 0 | 1 | Cloud -> Device | |
| V27 | Zone enabled | Integer | 0 | 1 | Cloud <-> Device | |
| V28 | Battery voltage | Double | 0 | 5 | Device -> Cloud | V |
| V29 | Low-battery threshold | Integer | 0 | 100 | Cloud -> Device | % |
| V30 | Diagnostics | String | | | Device -> Cloud | |

Important:
- V21 must be min 1 and max 15.
- V22, V23, and V26 are push buttons that send value 1.
- V28 must be Double and shown with 2 decimals in the widget.

## 3) Create event

In Template -> Events, add:
- Event code: low_moisture
- Name: Low moisture
- Event code: low_battery
- Name: Low battery

## 4) Build web dashboard sections

Open Template -> Web Dashboard and add widgets in this order.

### Section A: Overview
- Gauge -> V15 -> label Battery
- Value -> V28 -> Battery voltage (show 2 decimals, unit V)
- Slider -> V17 -> min 0 max 100 step 1
- Numeric Input (or Step Input) -> V16 -> min 5 max 1440
- Numeric Input -> V29 -> min 0 max 100 step 1
- Switch -> V20 -> off 0 on 1
- Switch -> V25 -> off 0 on 1
- Button (push) -> V26 sends 1
- Value/Text -> V30 diagnostics string

### Section B: Zones
- 15 Value widgets bound to V0..V14
- Labels: Zone 1 .. Zone 15
- Unit: %

### Section C: Calibration
- Slider -> V21 -> min 1 max 15 step 1
- Switch -> V27 -> off 0 on 1
- Value -> V24 (Live raw ADC)
- Value -> V18 (Dry raw)
- Value -> V19 (Wet raw)
- Button (push) -> V22 sends 1
- Button (push) -> V23 sends 1

### Section D: Diagnostics
- Value/Text -> V30 (Diagnostics)

## 5) Optional quick start

If your account supports JSON import, use blynk_console_template.json as blueprint. If import is not supported, create manually from this guide.

## 6) Validation checklist

1. Device online in Blynk Console.
2. V0..V14 update after wake report.
3. V20 set to 1 keeps calibration context active for controls.
4. V21 slider changes selected calibration zone.
5. V24 updates for selected zone.
6. V22 and V23 capture actions trigger updates on V18 and V19.
7. V28 shows decimal voltage (example 3.84 V, not 3).

## 7) Mobile app setup

For the matching phone dashboard, follow [BLYNK_MOBILE_DASHBOARD_SETUP.md](BLYNK_MOBILE_DASHBOARD_SETUP.md).
