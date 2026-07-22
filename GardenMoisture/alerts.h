#pragma once

#include "config.h"
#include <Preferences.h>
#include <time.h>

// RTC memory survives deep sleep; Preferences survives power loss.
RTC_DATA_ATTR uint32_t lastAlertDayKey[SENSOR_COUNT] = {0};

inline uint32_t currentLocalDayKey() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return 0;
  }
  // Unique key for each calendar day (local time).
  return (uint32_t)((timeinfo.tm_year + 1900) * 1000 + timeinfo.tm_yday);
}

inline bool syncDeviceTime() {
  setenv("TZ", TIMEZONE_TZ, 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  for (int i = 0; i < 20; i++) {
    if (currentLocalDayKey() != 0) {
      return true;
    }
    delay(250);
  }
  return false;
}

inline void alertsLoadFromFlash() {
  Preferences prefs;
  if (!prefs.begin("garden", true)) {
    return;
  }

  for (int i = 0; i < SENSOR_COUNT; i++) {
    char key[12];
    snprintf(key, sizeof(key), "alertDay%d", i);
    const uint32_t stored = prefs.getUInt(key, 0);
    if (stored > lastAlertDayKey[i]) {
      lastAlertDayKey[i] = stored;
    }
  }

  prefs.end();
}

inline void alertsSaveZone(int zoneIndex) {
  if (zoneIndex < 0 || zoneIndex >= SENSOR_COUNT) {
    return;
  }

  Preferences prefs;
  if (!prefs.begin("garden", false)) {
    return;
  }

  char key[12];
  snprintf(key, sizeof(key), "alertDay%d", zoneIndex);
  prefs.putUInt(key, lastAlertDayKey[zoneIndex]);
  prefs.end();
}

inline bool canAlertToday(int zoneIndex) {
  if (zoneIndex < 0 || zoneIndex >= SENSOR_COUNT) {
    return false;
  }

  const uint32_t today = currentLocalDayKey();
  if (today == 0) {
    return false;  // time not synced — skip rather than spam
  }

  return lastAlertDayKey[zoneIndex] != today;
}

inline void markAlertSentToday(int zoneIndex) {
  const uint32_t today = currentLocalDayKey();
  if (today == 0) {
    return;
  }

  lastAlertDayKey[zoneIndex] = today;
  alertsSaveZone(zoneIndex);
}
