/*************************************************************
  Garden Moisture Monitor v1.3
  - Arduino Nano ESP32, 15 capacitive sensors (2× CD4051)
  - 10000 mAh Li-ion, solar, wake every N minutes
  - Blynk IoT: moisture, battery, alerts, calibration, diagnostics

  Blynk template datastreams:
    V0–V14  zone moisture %                    (device → app)
    V15     battery %                          (device → app)
    V16     sleep interval (min)               (app → device)
    V17     alert threshold %                  (app → device)
    V18/V19 cal dry/wet raw for selected zone  (both)
    V20     calibration mode switch            (app → device)
    V21     zone select (1–15)                 (app → device)
    V22/V23 capture dry / capture wet buttons  (app → device)
    V24     live raw ADC for selected zone     (device → app)
    V25     stay-awake switch                  (app → device)
    V26     report-now button                  (app → device)
    V27     enable/disable selected zone       (both)
    V28     battery voltage (V)                (device → app)
    V29     low-battery threshold %            (app → device)
    V30     diagnostics string                 (device → app)

  Blynk events (Template → Events): low_moisture, low_battery

  Sync model: setting pins (V16/17/20/21/27/29/25) are pulled DOWN with
  Blynk.syncVirtual() on connect so the app is the source of truth. The
  device only writes telemetry UP. Settings are seeded up exactly once
  (provisioned flag) so widgets show correct initial values without ever
  clobbering later edits.
 *************************************************************/

#include "config.h"
#include "esp_sleep.h"
#include <Arduino.h>

#ifndef BLYNK_TEMPLATE_ID
#error "Set BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME in config.h (from Blynk Console)"
#endif

#define BLYNK_PRINT Serial
#define APP_DEBUG

#include "BlynkEdgent.h"
#include "calibration.h"
#include "sensors.h"
#include "alerts.h"

CalibrationSettings cal;

// Session (not persisted): keep it awake for live config / monitoring.
static bool stayAwake = false;

// Wake-cycle bookkeeping.
static bool reportedThisWake   = false;
static unsigned long bootMs         = 0;
static unsigned long connectedAtMs  = 0;
static unsigned long lastLiveReportMs = 0;
static bool moistureSmoothingInitialized = false;
static int smoothedMoisture[SENSOR_COUNT] = {0};

// ── Provisioning helper ──────────────────────────────────────────────────────
void applyHomeWifiProvisioning() {
#if HOME_WIFI_PROVISIONING
  const bool hasSsid = String(HOME_WIFI_SSID).length() > 0 && String(HOME_WIFI_SSID) != "YOUR_WIFI_SSID";
  const bool hasToken = String(HOME_BLYNK_TOKEN).length() == 32 && String(HOME_BLYNK_TOKEN) != "YOUR_32_CHAR_BLYNK_TOKEN";
  if (!hasSsid) {
    Serial.println(F("HOME_WIFI_PROVISIONING enabled, but SSID placeholder is not set."));
    return;
  }

  configStore = configDefault;
  CopyString(String(HOME_WIFI_SSID), configStore.wifiSSID);
  CopyString(String(HOME_WIFI_PASS), configStore.wifiPass);
  if (hasToken) {
    CopyString(String(HOME_BLYNK_TOKEN), configStore.cloudToken);
  } else {
    Serial.println(F("HOME_BLYNK_TOKEN not set; device will join Wi-Fi but cloud login may fail."));
  }
  configStore.setFlag(CONFIG_FLAG_VALID, true);
  config_save();
  Serial.println(F("Applied home Wi-Fi provisioning config."));
#endif
}

// ── Small helpers ────────────────────────────────────────────────────────────
int zoneIndexFromPin(long pinValue) {
  return (int)constrain(pinValue, 1, SENSOR_COUNT) - 1;
}

bool shouldStayAwake() {
  return stayAwake || cal.calMode || !ENABLE_DEEP_SLEEP;
}

const char* wakeReasonStr() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER: return "timer";
    case ESP_SLEEP_WAKEUP_EXT0:  return "button";
    case ESP_SLEEP_WAKEUP_EXT1:  return "button";
    default:                     return "power-on";
  }
}

// Push the selected zone's calibration/enable state to the app (display).
void pushZoneDisplay() {
  Blynk.virtualWrite(VPIN_CAL_DRY_RAW, cal.rawDry[cal.selectedZone]);
  Blynk.virtualWrite(VPIN_CAL_WET_RAW, cal.rawWet[cal.selectedZone]);
  Blynk.virtualWrite(VPIN_ZONE_ENABLED, cal.enabled[cal.selectedZone] ? 1 : 0);
}

// First-connect only: seed input widgets so they show the device's values.
void seedSettingsToApp() {
  Blynk.virtualWrite(VPIN_SLEEP_MINUTES, (int)cal.sleepMinutes);
  Blynk.virtualWrite(VPIN_ALERT_THRESHOLD, cal.alertThreshold);
  Blynk.virtualWrite(VPIN_CAL_MODE, cal.calMode ? 1 : 0);
  Blynk.virtualWrite(VPIN_CAL_ZONE, cal.selectedZone + 1);
  Blynk.virtualWrite(VPIN_LOW_BATT_THRESH, cal.lowBattThreshold);
  Blynk.virtualWrite(VPIN_STAY_AWAKE, stayAwake ? 1 : 0);
  pushZoneDisplay();
}

// Every subsequent connect: pull the app's stored input values down.
// Only GLOBAL, single-valued settings are synced down. Per-zone widgets
// (V18/V19 raws, V27 enable) are device-authoritative — they are pushed UP for
// the selected zone and edited live, because one widget maps to 15 backing
// values and a sync-down would be ambiguous (and would clobber the display).
void syncSettingsFromApp() {
  Blynk.syncVirtual(VPIN_SLEEP_MINUTES, VPIN_ALERT_THRESHOLD, VPIN_CAL_MODE,
                    VPIN_CAL_ZONE, VPIN_LOW_BATT_THRESH, VPIN_STAY_AWAKE);
}

// ── Calibration capture ──────────────────────────────────────────────────────
void captureCalibrationRaw(bool dryCapture) {
  sensorsPowerOn();
  const int raw = readRawMoisture(cal.selectedZone);
  sensorsPowerOff();

  if (dryCapture) {
    cal.rawDry[cal.selectedZone] = raw;
  } else {
    cal.rawWet[cal.selectedZone] = raw;
  }
  calibrationSaveZone(cal, cal.selectedZone);

  pushZoneDisplay();
  Blynk.virtualWrite(VPIN_CAL_RAW_LIVE, raw);

  Serial.printf("Captured %s for zone %d: raw=%d\n",
                dryCapture ? "DRY" : "WET", cal.selectedZone + 1, raw);
}

// ── Alerts ───────────────────────────────────────────────────────────────────
void checkLowMoistureAlerts(const int moisture[SENSOR_COUNT]) {
  if (!syncDeviceTime()) {
    Serial.println(F("Time sync failed — skipping moisture alerts this wake."));
    return;
  }

  for (int i = 0; i < SENSOR_COUNT; i++) {
    if (!cal.enabled[i]) continue;
    if (moisture[i] >= cal.alertThreshold) continue;

    if (!canAlertToday(i)) {
      Serial.printf("  Zone %d low (%d%%) — alert already sent today\n", i + 1, moisture[i]);
      continue;
    }

    const String msg = String("Zone ") + (i + 1) + " low: " + moisture[i] + "% (threshold " + cal.alertThreshold + "%)";
    Serial.println(msg);
    Blynk.logEvent(BLYNK_EVENT_LOW_MOISTURE, msg);
    markAlertSentToday(i);
  }
}

void checkLowBatteryAlert(int battPct) {
  if (battPct > cal.lowBattThreshold) return;
  if (!syncDeviceTime()) return;
  if (!canBattAlertToday()) return;

  const String msg = String("Battery low: ") + battPct + "% (threshold " + cal.lowBattThreshold + "%)";
  Serial.println(msg);
  Blynk.logEvent(BLYNK_EVENT_LOW_BATTERY, msg);
  markBattAlertSentToday();
}

// ── Diagnostics ──────────────────────────────────────────────────────────────
void publishDiagnostics(int battPct) {
  const unsigned long upMin = millis() / 60000UL;
  const long rssi = WiFi.RSSI();
  const uint32_t heap = ESP.getFreeHeap();

  char buf[160];
  snprintf(buf, sizeof(buf),
           "up %lum | RSSI %lddBm | heap %u | batt %d%% | wake %s | fw %s",
           upMin, rssi, (unsigned)heap, battPct, wakeReasonStr(), BLYNK_FIRMWARE_VERSION);
  Blynk.virtualWrite(VPIN_DIAGNOSTICS, buf);
}

// ── Full read + report ───────────────────────────────────────────────────────
void reportAll() {
  int raw[SENSOR_COUNT];
  int moisture[SENSOR_COUNT];
  long rawAccum[SENSOR_COUNT] = {0};
  int rawCycle[SENSOR_COUNT];

  const bool inCalibration = cal.calMode;
  const int configuredSweeps = inCalibration ? SENSOR_SWEEP_CYCLES_CAL : SENSOR_SWEEP_CYCLES_REPORT;
  const int configuredIntervalMs = inCalibration ? SENSOR_SWEEP_INTERVAL_MS_CAL : SENSOR_SWEEP_INTERVAL_MS_REPORT;
  const int sweeps = (configuredSweeps < 1) ? 1 : configuredSweeps;
  const int intervalMs = (configuredIntervalMs < 0) ? 0 : configuredIntervalMs;

  for (int sweep = 0; sweep < sweeps; sweep++) {
    // One full-zone sweep, then optional pause before the next sweep.
    readAllRaw(rawCycle);
    for (int i = 0; i < SENSOR_COUNT; i++) {
      rawAccum[i] += rawCycle[i];
    }
    if (sweep < sweeps - 1 && intervalMs > 0) {
      delay(intervalMs);
    }
  }

  for (int i = 0; i < SENSOR_COUNT; i++) {
    raw[i] = (int)(rawAccum[i] / sweeps);
    moisture[i] = rawToPercentForZone(raw[i], cal, i);
  }

  if (!inCalibration && MOISTURE_SMOOTHING_ENABLED) {
    const int alphaPct = constrain(MOISTURE_SMOOTHING_ALPHA_PCT, 0, 100);
    for (int i = 0; i < SENSOR_COUNT; i++) {
      if (!moistureSmoothingInitialized) {
        smoothedMoisture[i] = moisture[i];
      } else {
        smoothedMoisture[i] = (smoothedMoisture[i] * (100 - alphaPct) + moisture[i] * alphaPct + 50) / 100;
      }
      moisture[i] = constrain(smoothedMoisture[i], 0, 100);
    }
    moistureSmoothingInitialized = true;
  }

  // Moisture gauges always show %, even during calibration.
  for (int i = 0; i < SENSOR_COUNT; i++) {
    Blynk.virtualWrite(i, moisture[i]);
  }

  const float vBat  = readBatteryVoltage();
  const int battPct = batteryPercentFromVoltage(vBat);
  Blynk.virtualWrite(VPIN_BATTERY, battPct);
  Blynk.virtualWrite(VPIN_BATTERY_VOLTAGE, vBat);

  // Selected-zone calibration readback.
  pushZoneDisplay();
  Blynk.virtualWrite(VPIN_CAL_RAW_LIVE, raw[cal.selectedZone]);

  if (!cal.calMode) {
    checkLowMoistureAlerts(moisture);
  }
  checkLowBatteryAlert(battPct);
  publishDiagnostics(battPct);

  if (cal.calMode) {
    Serial.println(F("--- Calibration mode (raw ADC per zone) ---"));
    for (int i = 0; i < SENSOR_COUNT; i++) {
      Serial.printf("  Zone %2d: raw=%4d  dry=%4d  wet=%4d  -> %3d%%%s\n",
                    i + 1, raw[i], cal.rawDry[i], cal.rawWet[i], moisture[i],
                    cal.enabled[i] ? "" : "  [disabled]");
    }
  } else {
    Serial.println(F("--- Sensor report ---"));
    for (int i = 0; i < SENSOR_COUNT; i++) {
      Serial.printf("  Zone %2d: %3d%%%s\n", i + 1, moisture[i],
                    cal.enabled[i] ? "" : "  [disabled]");
    }
  }

  Serial.printf("  Battery: %d%% (%.2f V)\n", battPct, vBat);
  Serial.printf("  Alert threshold: %d%% | Low-batt: %d%%\n", cal.alertThreshold, cal.lowBattThreshold);
  Serial.printf("  Next sleep: %u min\n", cal.sleepMinutes);
}

// ── Deep sleep ───────────────────────────────────────────────────────────────
void enterDeepSleep() {
  if (!ENABLE_DEEP_SLEEP) {
    return;  // debug builds stay awake; handled by shouldStayAwake()
  }

  const uint64_t us = (uint64_t)cal.sleepMinutes * 60ULL * 1000000ULL;
  Serial.printf("Sleeping for %u minutes...\n", cal.sleepMinutes);
#ifdef BOARD_BUTTON_PIN
  Serial.printf("Wake button enabled on GPIO%d\n", BOARD_BUTTON_PIN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BOARD_BUTTON_PIN,
                               BOARD_BUTTON_ACTIVE_LOW ? 0 : 1);
#endif
  esp_sleep_enable_timer_wakeup(us);
  Serial.flush();
  esp_deep_sleep_start();
}

// Decide whether it is time to sleep. The MAX_AWAKE_MS cap is a hard battery
// guard: it forces sleep even if the cloud never connected.
void serviceSleep() {
  if (shouldStayAwake()) return;

  const bool windowElapsed = reportedThisWake && Blynk.connected() &&
                             (millis() - connectedAtMs > AWAKE_WINDOW_MS);
  const bool hardCap = (millis() - bootMs > MAX_AWAKE_MS);

  if (windowElapsed || hardCap) {
    if (hardCap && !reportedThisWake) {
      Serial.println(F("Never connected within MAX_AWAKE_MS — sleeping to save battery."));
    }
    enterDeepSleep();
  }
}

// ── Blynk pin handlers ───────────────────────────────────────────────────────
// NOTE: BLYNK_WRITE() must receive a numeric literal so it pastes into the
// library's BlynkWidgetWrite<N> handler. Keep these numbers in sync with the
// VPIN_* map in config.h.
BLYNK_WRITE(16) {  // VPIN_SLEEP_MINUTES
  const uint32_t m = clampSleepMinutes(param.asLong());
  if (m != cal.sleepMinutes) {
    cal.sleepMinutes = m;
    settingPutUInt("sleepmin", cal.sleepMinutes);
  }
}

BLYNK_WRITE(17) {  // VPIN_ALERT_THRESHOLD
  const long value = param.asLong();
  if (value >= 0 && value <= 100 && value != cal.alertThreshold) {
    cal.alertThreshold = (int)value;
    settingPutInt("alert", cal.alertThreshold);
  }
}

BLYNK_WRITE(29) {  // VPIN_LOW_BATT_THRESH
  const long value = param.asLong();
  if (value >= 0 && value <= 100 && value != cal.lowBattThreshold) {
    cal.lowBattThreshold = (int)value;
    settingPutInt("lowbatt", cal.lowBattThreshold);
  }
}

BLYNK_WRITE(20) {  // VPIN_CAL_MODE
  const bool on = param.asInt() != 0;
  if (on != cal.calMode) {
    cal.calMode = on;
    settingPutBool("calmode", cal.calMode);
    Serial.println(cal.calMode ? F("Calibration mode ON") : F("Calibration mode OFF"));
    if (!cal.calMode) {
      connectedAtMs = millis();  // restart the awake window before sleeping
    }
  }
}

BLYNK_WRITE(25) {  // VPIN_STAY_AWAKE
  stayAwake = param.asInt() != 0;
  Serial.println(stayAwake ? F("Stay-awake ON") : F("Stay-awake OFF"));
  if (!stayAwake) {
    connectedAtMs = millis();  // restart the awake window before sleeping
  }
}

BLYNK_WRITE(21) {  // VPIN_CAL_ZONE
  const int z = zoneIndexFromPin(param.asLong());
  if (z != cal.selectedZone) {
    cal.selectedZone = z;
    settingPutInt("zone", cal.selectedZone);
  }
  pushZoneDisplay();
}

BLYNK_WRITE(27) {  // VPIN_ZONE_ENABLED
  const bool en = param.asInt() != 0;
  if (en != cal.enabled[cal.selectedZone]) {
    cal.enabled[cal.selectedZone] = en;
    calibrationSaveZone(cal, cal.selectedZone);
    Serial.printf("Zone %d %s\n", cal.selectedZone + 1, en ? "enabled" : "disabled");
  }
}

BLYNK_WRITE(18) {  // VPIN_CAL_DRY_RAW
  const long value = param.asLong();
  if (value >= 0 && value <= 4095) {
    cal.rawDry[cal.selectedZone] = (int)value;
    calibrationSaveZone(cal, cal.selectedZone);
  }
}

BLYNK_WRITE(19) {  // VPIN_CAL_WET_RAW
  const long value = param.asLong();
  if (value >= 0 && value <= 4095) {
    cal.rawWet[cal.selectedZone] = (int)value;
    calibrationSaveZone(cal, cal.selectedZone);
  }
}

BLYNK_WRITE(22) {  // VPIN_CAPTURE_DRY
  if (param.asInt() == 1) {
    captureCalibrationRaw(true);
    Blynk.virtualWrite(VPIN_CAPTURE_DRY, 0);  // reset momentary button
  }
}

BLYNK_WRITE(23) {  // VPIN_CAPTURE_WET
  if (param.asInt() == 1) {
    captureCalibrationRaw(false);
    Blynk.virtualWrite(VPIN_CAPTURE_WET, 0);
  }
}

BLYNK_WRITE(26) {  // VPIN_REPORT_NOW
  if (param.asInt() == 1) {
    Serial.println(F("Report-now requested."));
    reportAll();
    lastLiveReportMs = millis();
    Blynk.virtualWrite(VPIN_REPORT_NOW, 0);
  }
}

BLYNK_CONNECTED() {
  connectedAtMs = millis();

  if (!cal.provisioned) {
    seedSettingsToApp();
    cal.provisioned = true;
    settingPutBool("prov", true);
  } else {
    syncSettingsFromApp();
  }

  reportAll();
  reportedThisWake = true;
  lastLiveReportMs = millis();
}

// ── Arduino entry points ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);

  calibrationLoad(cal);
  alertsLoadFromFlash();

  sensorsBegin();  // sets ADC resolution + attenuation, mux pins, power switch

  applyHomeWifiProvisioning();
  BlynkEdgent.begin();

  bootMs = millis();

  Serial.println(F("Garden Moisture Monitor ready."));
  Serial.printf("Wake reason: %s | Sleep interval: %u min | Alert threshold: %d%%\n",
                wakeReasonStr(), cal.sleepMinutes, cal.alertThreshold);
}

void loop() {
  BlynkEdgent.run();

  // Live raw for the selected zone while calibrating.
  if (cal.calMode && Blynk.connected()) {
    static unsigned long lastCalMs = 0;
    if (millis() - lastCalMs > 5000) {
      lastCalMs = millis();
      sensorsPowerOn();
      const int raw = readRawMoisture(cal.selectedZone);
      sensorsPowerOff();
      Blynk.virtualWrite(VPIN_CAL_RAW_LIVE, raw);
    }
  }

  // Periodic re-report while we are deliberately staying awake.
  if (shouldStayAwake() && reportedThisWake && Blynk.connected()) {
    if (millis() - lastLiveReportMs > LIVE_REPORT_INTERVAL_MS) {
      lastLiveReportMs = millis();
      reportAll();
    }
  }

  serviceSleep();
}
