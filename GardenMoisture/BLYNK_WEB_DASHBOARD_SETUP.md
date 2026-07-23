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
| V25 | WiFi RSSI | Integer | -120 | 0 | Device -> Cloud | dBm |
| V26 | Last report | String | | | Device -> Cloud | |
| V27 | Last error | String | | | Device -> Cloud | |

Important:
- V21 must be min 1 and max 15.
- V22 and V23 are push buttons that send value 1.

## 3) Create event

In Template -> Events, add:
- Event code: low_moisture
- Name: Low moisture

## 4) Build web dashboard sections

Open Template -> Web Dashboard and add widgets in this order.

### Section A: Overview
- Gauge -> V15 -> label Battery
- Slider -> V17 -> min 0 max 100 step 1
- Numeric Input (or Step Input) -> V16 -> min 5 max 1440
- Switch -> V20 -> off 0 on 1

### Section B: Zones
- 15 Value widgets bound to V0..V14
- Labels: Zone 1 .. Zone 15
- Unit: %

### Section C: Calibration
- Slider -> V21 -> min 1 max 15 step 1
- Value -> V24 (Live raw ADC)
- Value -> V18 (Dry raw)
- Value -> V19 (Wet raw)
- Button (push) -> V22 sends 1
- Button (push) -> V23 sends 1

### Section D: Diagnostics
- Value -> V25 (WiFi RSSI)
- Value -> V26 (Last report)
- Value -> V27 (Last error)

## 5) Optional quick start

If your account supports JSON import, use blynk_console_template.json as blueprint. If import is not supported, create manually from this guide.

## 6) Validation checklist

1. Device online in Blynk Console.
2. V0..V14 update after wake report.
3. V20 set to 1 keeps calibration context active for controls.
4. V21 slider changes selected calibration zone.
5. V24 updates for selected zone.
6. V22 and V23 capture actions trigger updates on V18 and V19.

## 7) Mobile app setup

For the matching phone dashboard, follow [BLYNK_MOBILE_DASHBOARD_SETUP.md](BLYNK_MOBILE_DASHBOARD_SETUP.md).
