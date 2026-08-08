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
// Direction legend: [↓ app→device input]  [↑ device→app telemetry]  [↕ both]
// V0–V14  moisture % per zone                                           [↑]
#define VPIN_BATTERY          15   // battery %                          [↑]
#define VPIN_SLEEP_MINUTES    16   // wake interval, minutes             [↓]
#define VPIN_ALERT_THRESHOLD  17   // integer 0–100, alert when below    [↓]
#define VPIN_CAL_DRY_RAW      18   // per-zone dry raw (display + edit)   [↕]
#define VPIN_CAL_WET_RAW      19   // per-zone wet raw (display + edit)   [↕]
#define VPIN_CAL_MODE         20   // 0 = normal, 1 = calibration mode    [↓]
#define VPIN_CAL_ZONE         21   // 1–15, zone selected for calibration [↓]
#define VPIN_CAPTURE_DRY      22   // button: capture current raw as dry  [↓]
#define VPIN_CAPTURE_WET      23   // button: capture current raw as wet  [↓]
#define VPIN_CAL_RAW_LIVE     24   // live raw ADC for selected zone      [↑]
#define VPIN_STAY_AWAKE       25   // 1 = never deep-sleep (live config)  [↓]
#define VPIN_REPORT_NOW       26   // button: read & report immediately   [↓]
#define VPIN_ZONE_ENABLED     27   // enable/disable selected zone alerts [↕]
#define VPIN_BATTERY_VOLTAGE  28   // battery voltage (V)                 [↑]
#define VPIN_LOW_BATT_THRESH  29   // low-battery alert threshold %       [↓]
#define VPIN_DIAGNOSTICS      30   // status string (uptime/RSSI/heap…)   [↑]
#define VPIN_PWR_SWITCHING    31   // 1 = toggle sensor rail per read      [↓]

// Blynk Console → Template → Events → create event codes:
#define BLYNK_EVENT_LOW_MOISTURE "low_moisture"
#define BLYNK_EVENT_LOW_BATTERY  "low_battery"

// ── Power ──────────────────────────────────────────────────────────────────────
#define ENABLE_DEEP_SLEEP true
#define DEFAULT_SLEEP_MINUTES 30     // wake every 30 minutes
#define MIN_SLEEP_MINUTES 5
#define MAX_SLEEP_MINUTES 1440

// How long to stay online after connecting so dashboard commands land,
// then deep-sleep. Also a hard wall-clock cap that forces sleep even if
// the cloud never connects (critical battery-drain guard).
#define AWAKE_WINDOW_MS   45000UL    // grace window after a successful connect
#define MAX_AWAKE_MS      120000UL   // absolute cap; force sleep no matter what
#define LIVE_REPORT_INTERVAL_MS 10000UL  // re-report cadence while kept awake

// 10000 mAh Li-ion (3.0 V cutoff → 4.2 V full)
#define BATTERY_ADC_PIN         A7
#define BATTERY_DIVIDER_RATIO   2.0f
#define BATTERY_V_MIN           3.00f
#define BATTERY_V_MAX           4.20f
#define BATTERY_ADC_SETTLE_MS   40       // settle time before single battery ADC read
#define BATTERY_SMOOTHING_ENABLED true
#define BATTERY_SMOOTHING_ALPHA_PCT 25   // 0=hold previous, 100=no smoothing
#define DEFAULT_LOW_BATT_THRESHOLD 15    // % — alert (once/day) when at or below

// ── Sensor sampling / filtering ──────────────────────────────────────────────
#define SENSOR_SAMPLES  12    // raw ADC samples taken per zone read
#define SENSOR_DISCARD  2     // drop this many highest AND lowest (trimmed mean)
#define SENSOR_SWEEP_CYCLES_REPORT 3        // full-zone sweeps for normal reports
#define SENSOR_SWEEP_INTERVAL_MS_REPORT 2000  // pause between report sweeps (ms)
#define SENSOR_SWEEP_CYCLES_CAL 2           // full-zone sweeps in calibration mode
#define SENSOR_SWEEP_INTERVAL_MS_CAL 250    // pause between calibration sweeps (ms)

// Firmware smoothing after raw->percent conversion (normal mode only).
// 0 = hold previous value, 100 = no smoothing.
#define MOISTURE_SMOOTHING_ENABLED true
#define MOISTURE_SMOOTHING_ALPHA_PCT 40

// ── Default calibration (capacitive: wet = lower ADC) ────────────────────────
#define DEFAULT_RAW_DRY  3200
#define DEFAULT_RAW_WET  1400
#define DEFAULT_ALERT_THRESHOLD 25   // %

// Local timezone for daily alert limit (POSIX TZ string)
#define TIMEZONE_TZ "CST6CDT"

#define BLYNK_FIRMWARE_VERSION "1.3.0"

// ── CD74HC4067 multiplexer (16-ch, 15 sensors on A0) ───────────────────────
#define MUX_SIG_PIN  A0
#define MUX_EN_PIN   D10   // active-low enable
#define MUX_S0_PIN   D2
#define MUX_S1_PIN   D3
#define MUX_S2_PIN   D4
#define MUX_S3_PIN   D5

#define SENSOR_POWER_PIN  D8
#define USE_SENSOR_POWER_SWITCH true
#define DEFAULT_SENSOR_POWER_SWITCHING_ENABLED true

// Sensor/mux rail power switch control.
// AO3401 P-MOS high-side switch is typically gate-active-low:
//   LOW  -> rail ON
//   HIGH -> rail OFF
#define SENSOR_POWER_ACTIVE_LOW true
#define SENSOR_POWER_ON_DELAY_MS 50
