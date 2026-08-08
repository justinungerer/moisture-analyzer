#pragma once

#include "config.h"
#include <Preferences.h>
#include <Arduino.h>

// All persistent device state lives here. Loaded once at boot and written
// back only when something actually changes (to spare NVS flash wear).
struct CalibrationSettings {
  int  rawDry[SENSOR_COUNT];
  int  rawWet[SENSOR_COUNT];
  bool enabled[SENSOR_COUNT];   // false = ignore this zone for alerts
  int  alertThreshold;          // % below which a zone alerts
  int  lowBattThreshold;        // % at/below which battery alerts
  uint32_t sleepMinutes;        // deep-sleep interval
  int  selectedZone;            // 0-based zone shown/edited in the app
  bool calMode;                 // calibration mode (device stays awake)
  bool sensorPowerSwitchingEnabled;  // true=toggled per read, false=always on
  bool provisioned;             // have we seeded widget values up once?
};

inline Preferences prefs;

inline void calibrationLoadDefaults(CalibrationSettings& cal) {
  cal.alertThreshold  = DEFAULT_ALERT_THRESHOLD;
  cal.lowBattThreshold = DEFAULT_LOW_BATT_THRESHOLD;
  cal.sleepMinutes    = DEFAULT_SLEEP_MINUTES;
  cal.selectedZone    = 0;
  cal.calMode         = false;
  cal.sensorPowerSwitchingEnabled = DEFAULT_SENSOR_POWER_SWITCHING_ENABLED;
  cal.provisioned     = false;
  for (int i = 0; i < SENSOR_COUNT; i++) {
    cal.rawDry[i]  = DEFAULT_RAW_DRY;
    cal.rawWet[i]  = DEFAULT_RAW_WET;
    cal.enabled[i] = true;
  }
}

inline uint32_t clampSleepMinutes(long minutes) {
  if (minutes < MIN_SLEEP_MINUTES) minutes = MIN_SLEEP_MINUTES;
  if (minutes > MAX_SLEEP_MINUTES) minutes = MAX_SLEEP_MINUTES;
  return (uint32_t)minutes;
}

inline void calibrationLoad(CalibrationSettings& cal) {
  calibrationLoadDefaults(cal);

  if (!prefs.begin("garden", true)) {
    return;
  }

  cal.alertThreshold   = prefs.getInt("alert", DEFAULT_ALERT_THRESHOLD);
  cal.lowBattThreshold = prefs.getInt("lowbatt", DEFAULT_LOW_BATT_THRESHOLD);
  cal.sleepMinutes     = clampSleepMinutes(prefs.getUInt("sleepmin", DEFAULT_SLEEP_MINUTES));
  cal.selectedZone     = constrain(prefs.getInt("zone", 0), 0, SENSOR_COUNT - 1);
  cal.calMode          = prefs.getBool("calmode", false);
  cal.sensorPowerSwitchingEnabled = prefs.getBool("pwrsw", DEFAULT_SENSOR_POWER_SWITCHING_ENABLED);
  cal.provisioned      = prefs.getBool("prov", false);

  for (int i = 0; i < SENSOR_COUNT; i++) {
    char keyDry[8], keyWet[8], keyEn[8];
    snprintf(keyDry, sizeof(keyDry), "dry%d", i);
    snprintf(keyWet, sizeof(keyWet), "wet%d", i);
    snprintf(keyEn,  sizeof(keyEn),  "en%d",  i);
    cal.rawDry[i]  = prefs.getInt(keyDry, DEFAULT_RAW_DRY);
    cal.rawWet[i]  = prefs.getInt(keyWet, DEFAULT_RAW_WET);
    cal.enabled[i] = prefs.getBool(keyEn, true);
  }

  prefs.end();
}

inline void calibrationSave(const CalibrationSettings& cal) {
  if (!prefs.begin("garden", false)) {
    return;
  }

  prefs.putInt("alert", cal.alertThreshold);
  prefs.putInt("lowbatt", cal.lowBattThreshold);
  prefs.putUInt("sleepmin", cal.sleepMinutes);
  prefs.putInt("zone", cal.selectedZone);
  prefs.putBool("calmode", cal.calMode);
  prefs.putBool("pwrsw", cal.sensorPowerSwitchingEnabled);
  prefs.putBool("prov", cal.provisioned);

  for (int i = 0; i < SENSOR_COUNT; i++) {
    char keyDry[8], keyWet[8], keyEn[8];
    snprintf(keyDry, sizeof(keyDry), "dry%d", i);
    snprintf(keyWet, sizeof(keyWet), "wet%d", i);
    snprintf(keyEn,  sizeof(keyEn),  "en%d",  i);
    prefs.putInt(keyDry, cal.rawDry[i]);
    prefs.putInt(keyWet, cal.rawWet[i]);
    prefs.putBool(keyEn, cal.enabled[i]);
  }

  prefs.end();
}

inline void calibrationSaveZone(const CalibrationSettings& cal, int zoneIndex) {
  if (zoneIndex < 0 || zoneIndex >= SENSOR_COUNT) {
    return;
  }

  if (!prefs.begin("garden", false)) {
    return;
  }

  char keyDry[8], keyWet[8], keyEn[8];
  snprintf(keyDry, sizeof(keyDry), "dry%d", zoneIndex);
  snprintf(keyWet, sizeof(keyWet), "wet%d", zoneIndex);
  snprintf(keyEn,  sizeof(keyEn),  "en%d",  zoneIndex);
  prefs.putInt(keyDry, cal.rawDry[zoneIndex]);
  prefs.putInt(keyWet, cal.rawWet[zoneIndex]);
  prefs.putBool(keyEn, cal.enabled[zoneIndex]);
  prefs.end();
}

// Persist a single scalar setting without rewriting the whole struct.
inline void settingPutInt(const char* key, int value) {
  if (!prefs.begin("garden", false)) return;
  prefs.putInt(key, value);
  prefs.end();
}
inline void settingPutUInt(const char* key, uint32_t value) {
  if (!prefs.begin("garden", false)) return;
  prefs.putUInt(key, value);
  prefs.end();
}
inline void settingPutBool(const char* key, bool value) {
  if (!prefs.begin("garden", false)) return;
  prefs.putBool(key, value);
  prefs.end();
}

inline int rawToPercentForZone(int raw, const CalibrationSettings& cal, int zoneIndex) {
  const int dry = cal.rawDry[zoneIndex];
  const int wet = cal.rawWet[zoneIndex];

  if (dry == wet) {
    return 0;
  }

  if (dry > wet) {
    if (raw >= dry) return 0;
    if (raw <= wet) return 100;
    const long pct = ((long)dry - raw) * 100L / ((long)dry - wet);
    return (int)constrain(pct, 0, 100);
  }

  // Inverted sensor wiring: wet reads higher than dry.
  if (raw <= dry) return 0;
  if (raw >= wet) return 100;
  const long pct = ((long)raw - dry) * 100L / ((long)wet - dry);
  return (int)constrain(pct, 0, 100);
}
