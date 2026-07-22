#pragma once

#include "config.h"
#include <Preferences.h>
#include <Arduino.h>

struct CalibrationSettings {
  int rawDry[SENSOR_COUNT];
  int rawWet[SENSOR_COUNT];
  int alertThreshold;
  bool calMode;
};

inline Preferences prefs;

inline void calibrationLoadDefaults(CalibrationSettings& cal) {
  cal.alertThreshold = DEFAULT_ALERT_THRESHOLD;
  cal.calMode = false;
  for (int i = 0; i < SENSOR_COUNT; i++) {
    cal.rawDry[i] = DEFAULT_RAW_DRY;
    cal.rawWet[i] = DEFAULT_RAW_WET;
  }
}

inline void calibrationLoad(CalibrationSettings& cal) {
  calibrationLoadDefaults(cal);

  if (!prefs.begin("garden", true)) {
    return;
  }

  cal.alertThreshold = prefs.getInt("alert", DEFAULT_ALERT_THRESHOLD);
  cal.calMode = prefs.getBool("calmode", false);

  for (int i = 0; i < SENSOR_COUNT; i++) {
    char keyDry[8];
    char keyWet[8];
    snprintf(keyDry, sizeof(keyDry), "dry%d", i);
    snprintf(keyWet, sizeof(keyWet), "wet%d", i);
    cal.rawDry[i] = prefs.getInt(keyDry, DEFAULT_RAW_DRY);
    cal.rawWet[i] = prefs.getInt(keyWet, DEFAULT_RAW_WET);
  }

  prefs.end();
}

inline void calibrationSave(const CalibrationSettings& cal) {
  if (!prefs.begin("garden", false)) {
    return;
  }

  prefs.putInt("alert", cal.alertThreshold);
  prefs.putBool("calmode", cal.calMode);

  for (int i = 0; i < SENSOR_COUNT; i++) {
    char keyDry[8];
    char keyWet[8];
    snprintf(keyDry, sizeof(keyDry), "dry%d", i);
    snprintf(keyWet, sizeof(keyWet), "wet%d", i);
    prefs.putInt(keyDry, cal.rawDry[i]);
    prefs.putInt(keyWet, cal.rawWet[i]);
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

  char keyDry[8];
  char keyWet[8];
  snprintf(keyDry, sizeof(keyDry), "dry%d", zoneIndex);
  snprintf(keyWet, sizeof(keyWet), "wet%d", zoneIndex);
  prefs.putInt(keyDry, cal.rawDry[zoneIndex]);
  prefs.putInt(keyWet, cal.rawWet[zoneIndex]);
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
