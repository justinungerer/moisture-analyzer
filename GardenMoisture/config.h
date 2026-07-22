#pragma once

// ── Blynk template (paste from Blynk Console → Template → Firmware configuration) ──
#define BLYNK_TEMPLATE_ID   "TMPL2c6XdcWxv"
#define BLYNK_TEMPLATE_NAME "Soil Moisture Monitoring"

// Home-WiFi-first provisioning mode.
// Fill SSID/PASS to make the device join your LAN immediately on boot.
// HOME_BLYNK_TOKEN is optional if you only care about Wi-Fi join first.
#define HOME_WIFI_PROVISIONING true
#define HOME_WIFI_SSID "YOUR_WIFI_SSID"
#define HOME_WIFI_PASS "YOUR_WIFI_PASSWORD"
#define HOME_BLYNK_TOKEN "YOUR_32_CHAR_BLYNK_TOKEN"

#define SENSOR_COUNT 15

// ── Blynk virtual pins ─────────────────────────────────────────────────────────
// V0–V14  moisture % per zone
#define VPIN_BATTERY          15
#define VPIN_SLEEP_MINUTES    16
#define VPIN_ALERT_THRESHOLD  17   // integer 0–100, alert when zone drops below
#define VPIN_CAL_DRY_RAW      18   // global/per-zone dry raw (display + manual edit)
#define VPIN_CAL_WET_RAW      19
#define VPIN_CAL_MODE         20   // 0 = normal, 1 = calibration mode
#define VPIN_CAL_ZONE         21   // 1–15, zone selected for calibration
#define VPIN_CAPTURE_DRY      22   // button: capture current raw as dry
#define VPIN_CAPTURE_WET      23   // button: capture current raw as wet
#define VPIN_CAL_RAW_LIVE     24   // live raw ADC for selected zone (device → app)

// Blynk Console → Template → Events → create event code: low_moisture
#define BLYNK_EVENT_LOW_MOISTURE "low_moisture"

// ── Power ──────────────────────────────────────────────────────────────────────
#define ENABLE_DEEP_SLEEP true
#define DEFAULT_SLEEP_MINUTES 30     // wake every 30 minutes

// 10000 mAh Li-ion (3.0 V cutoff → 4.2 V full)
#define BATTERY_ADC_PIN         A7
#define BATTERY_DIVIDER_RATIO   2.0f
#define BATTERY_V_MIN           3.00f
#define BATTERY_V_MAX           4.20f

// ── Default calibration (capacitive: wet = lower ADC) ────────────────────────
#define DEFAULT_RAW_DRY  3200
#define DEFAULT_RAW_WET  1400
#define DEFAULT_ALERT_THRESHOLD 25   // %

// Local timezone for daily alert limit (POSIX TZ string)
#define TIMEZONE_TZ "CST6CDT"

#define BLYNK_FIRMWARE_VERSION "1.2.0"

// ── CD4051 multiplexers (2×8 → 15 sensors on A0) ───────────────────────────
#define MUX_SIG_PIN    A0
#define MUX1_INH_PIN   D10
#define MUX1_S0_PIN    D2
#define MUX1_S1_PIN    D3
#define MUX1_S2_PIN    D4
#define MUX2_INH_PIN   D9
#define MUX2_S0_PIN    D5
#define MUX2_S1_PIN    D6
#define MUX2_S2_PIN    D7

#define SENSOR_POWER_PIN  D8
#define USE_SENSOR_POWER_SWITCH true
