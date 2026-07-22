/*************************************************************
  Garden Moisture Monitor v1.1
  - Arduino Nano ESP32, 15 capacitive sensors (2× CD4051)
  - 10000 mAh Li-ion, solar, wake every 30 minutes
  - Blynk IoT: moisture, battery, low-moisture alerts (max 1/zone/day), calibration

  Blynk template datastreams:
    V0–V14   zone moisture %
    V15      battery %
    V16      sleep interval (min)
    V17      alert threshold %
    V18–V19  cal dry/wet raw for selected zone
    V20      calibration mode switch
    V21      zone select (1–15)
    V22–V23  capture dry / capture wet buttons
    V24      live raw ADC for selected zone

  Blynk event (Template → Events): code = low_moisture
 *************************************************************/

#include "config.h"
#include "esp_sleep.h"
#include <Arduino.h>

// Early boot serial marker removed: calling Serial before Arduino
// core init can block startup. Use setup() prints instead.

#ifndef BLYNK_TEMPLATE_ID
#error "Set BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME in config.h (from Blynk Console)"
#endif

#define BLYNK_PRINT Serial
#define APP_DEBUG

// Real includes (restore full implementations)
#include "BlynkEdgent.h"
#include "calibration.h"
#include "sensors.h"
#include "alerts.h"

RTC_DATA_ATTR uint32_t sleepMinutes = DEFAULT_SLEEP_MINUTES;

CalibrationSettings cal;
int selectedZone = 0;  // 0-based index

static bool reportedThisWake = false;
static unsigned long connectedAtMs = 0;

void applyHomeWifiProvisioning() {
#if HOME_WIFI_PROVISIONING
  const bool hasSsid = String(HOME_WIFI_SSID).length() > 0 && String(HOME_WIFI_SSID) != "YOUR_WIFI_SSID";
  const bool hasToken = String(HOME_BLYNK_TOKEN).length() == 32 && String(HOME_BLYNK_TOKEN) != "YOUR_32_CHAR_BLYNK_TOKEN";
  if (!hasSsid) {
    Serial.println(F("HOME_WIFI_PROVISIONING enabled, but SSID placeholder is not set."));
    return;
  }

  // Preload Edgent config so device joins home Wi-Fi instead of waiting in AP mode.
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

int zoneIndexFromPin(long pinValue) {
  return (int)constrain(pinValue, 1, SENSOR_COUNT) - 1;
}

void syncCalibrationToApp() {
  Blynk.virtualWrite(VPIN_ALERT_THRESHOLD, cal.alertThreshold);
  Blynk.virtualWrite(VPIN_CAL_MODE, cal.calMode ? 1 : 0);
  Blynk.virtualWrite(VPIN_CAL_ZONE, selectedZone + 1);
  Blynk.virtualWrite(VPIN_CAL_DRY_RAW, cal.rawDry[selectedZone]);
  Blynk.virtualWrite(VPIN_CAL_WET_RAW, cal.rawWet[selectedZone]);
}

void captureCalibrationRaw(bool dryCapture) {
  sensorsPowerOn();
  const int raw = readRawMoisture(selectedZone);
  sensorsPowerOff();

  if (dryCapture) {
    cal.rawDry[selectedZone] = raw;
  } else {
    cal.rawWet[selectedZone] = raw;
  }

  calibrationSaveZone(cal, selectedZone);

  Blynk.virtualWrite(VPIN_CAL_DRY_RAW, cal.rawDry[selectedZone]);
  Blynk.virtualWrite(VPIN_CAL_WET_RAW, cal.rawWet[selectedZone]);
  Blynk.virtualWrite(VPIN_CAL_RAW_LIVE, raw);

  Serial.printf("Captured %s for zone %d: raw=%d\n",
                dryCapture ? "DRY" : "WET", selectedZone + 1, raw);
}

void checkLowMoistureAlerts(const int moisture[SENSOR_COUNT]) {
  if (!syncDeviceTime()) {
    Serial.println(F("Time sync failed — skipping alerts this wake."));
    return;
  }

  for (int i = 0; i < SENSOR_COUNT; i++) {
    if (moisture[i] >= cal.alertThreshold) {
      continue;
    }

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

void reportAll() {
  int raw[SENSOR_COUNT];
  int moisture[SENSOR_COUNT];

  readAllRaw(raw);
  for (int i = 0; i < SENSOR_COUNT; i++) {
    moisture[i] = rawToPercentForZone(raw[i], cal, i);
  }

  if (cal.calMode) {
    // Calibration mode: show live raw on dashboard, moisture pins show raw ADC.
    for (int i = 0; i < SENSOR_COUNT; i++) {
      Blynk.virtualWrite(i, raw[i]);
    }
    Blynk.virtualWrite(VPIN_CAL_RAW_LIVE, raw[selectedZone]);
    Blynk.virtualWrite(VPIN_CAL_DRY_RAW, cal.rawDry[selectedZone]);
    Blynk.virtualWrite(VPIN_CAL_WET_RAW, cal.rawWet[selectedZone]);

    Serial.println(F("--- Calibration mode (pins V0-V14 = raw ADC) ---"));
    for (int i = 0; i < SENSOR_COUNT; i++) {
      Serial.printf("  Zone %2d: raw=%4d  dry=%4d  wet=%4d  -> %3d%%\n",
                    i + 1, raw[i], cal.rawDry[i], cal.rawWet[i], moisture[i]);
    }
  } else {
    for (int i = 0; i < SENSOR_COUNT; i++) {
      Blynk.virtualWrite(i, moisture[i]);
    }
    checkLowMoistureAlerts(moisture);

    Serial.println(F("--- Sensor report ---"));
    for (int i = 0; i < SENSOR_COUNT; i++) {
      Serial.printf("  Zone %2d: %3d%%\n", i + 1, moisture[i]);
    }
  }

  const int battery = readBatteryPercent();
  Blynk.virtualWrite(VPIN_BATTERY, battery);
  Blynk.virtualWrite(VPIN_SLEEP_MINUTES, (int)sleepMinutes);
  syncCalibrationToApp();

  Serial.printf("  Battery: %d%%\n", battery);
  Serial.printf("  Alert threshold: %d%%\n", cal.alertThreshold);
  Serial.printf("  Next sleep: %u min\n", sleepMinutes);
}

void enterDeepSleep() {
  if (!ENABLE_DEEP_SLEEP) {
    Serial.println(F("Deep sleep disabled (debug mode)."));
    return;
  }

  if (cal.calMode) {
    Serial.println(F("Calibration mode active — staying awake. Turn off V20 to resume sleep."));
    return;
  }

  calibrationSave(cal);

  const uint64_t us = (uint64_t)sleepMinutes * 60ULL * 1000000ULL;
  Serial.printf("Sleeping for %u minutes...\n", sleepMinutes);
#ifdef BOARD_BUTTON_PIN
  Serial.printf("Wake button enabled on GPIO%d\n", BOARD_BUTTON_PIN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BOARD_BUTTON_PIN,
                               BOARD_BUTTON_ACTIVE_LOW ? 0 : 1);
#endif
  esp_sleep_enable_timer_wakeup(us);
  Serial.flush();
  esp_deep_sleep_start();
}

// ── Blynk pin handlers ───────────────────────────────────────────────────────
BLYNK_WRITE(16) {
  const long minutes = param.asLong();
  if (minutes >= 5 && minutes <= 1440) {
    sleepMinutes = (uint32_t)minutes;
  }
}

BLYNK_WRITE(17) {
  const long value = param.asLong();
  if (value >= 0 && value <= 100) {
    cal.alertThreshold = (int)value;
    calibrationSave(cal);
  }
}

BLYNK_WRITE(20) {
  cal.calMode = param.asInt() != 0;
  calibrationSave(cal);
  Serial.println(cal.calMode ? F("Calibration mode ON") : F("Calibration mode OFF"));
}

BLYNK_WRITE(21) {
  selectedZone = zoneIndexFromPin(param.asLong());
  syncCalibrationToApp();
}

BLYNK_WRITE(18) {
  const long value = param.asLong();
  if (value >= 0 && value <= 4095) {
    cal.rawDry[selectedZone] = (int)value;
    calibrationSaveZone(cal, selectedZone);
  }
}

BLYNK_WRITE(19) {
  const long value = param.asLong();
  if (value >= 0 && value <= 4095) {
    cal.rawWet[selectedZone] = (int)value;
    calibrationSaveZone(cal, selectedZone);
  }
}

BLYNK_WRITE(22) {
  if (param.asInt() == 1) {
    captureCalibrationRaw(true);
  }
}

BLYNK_WRITE(23) {
  if (param.asInt() == 1) {
    captureCalibrationRaw(false);
  }
}

BLYNK_CONNECTED() {
  connectedAtMs = millis();
  syncCalibrationToApp();
  reportAll();
  reportedThisWake = true;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  analogReadResolution(12);
  calibrationLoad(cal);
  alertsLoadFromFlash();
  selectedZone = 0;
  sleepMinutes = DEFAULT_SLEEP_MINUTES;

  sensorsBegin();

  applyHomeWifiProvisioning();
  BlynkEdgent.begin();

  Serial.println(F("Garden Moisture Monitor ready."));
  Serial.printf("Sleep interval: %u min | Alert threshold: %d%%\n",
                sleepMinutes, cal.alertThreshold);
}

void loop() {
  BlynkEdgent.run();

  if (cal.calMode && Blynk.connected()) {
    static unsigned long lastCalMs = 0;
    if (millis() - lastCalMs > 5000) {
      lastCalMs = millis();
      sensorsPowerOn();
      const int raw = readRawMoisture(selectedZone);
      sensorsPowerOff();
      Blynk.virtualWrite(VPIN_CAL_RAW_LIVE, raw);
    }
  }

  if (reportedThisWake) {
    const bool shouldSleep = (millis() - connectedAtMs > 3000) || !Blynk.connected();
    if (shouldSleep) {
      enterDeepSleep();
    }
  }
}
