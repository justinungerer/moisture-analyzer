#pragma once

#include "config.h"
#include "calibration.h"
#include <Arduino.h>

inline void setMuxChannel(bool useMux2, uint8_t channel) {
  channel &= 0x07;

  digitalWrite(MUX1_INH_PIN, useMux2 ? HIGH : LOW);
  digitalWrite(MUX2_INH_PIN, useMux2 ? LOW : HIGH);

  digitalWrite(MUX1_S0_PIN, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(MUX1_S1_PIN, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(MUX1_S2_PIN, (channel & 0x04) ? HIGH : LOW);

  digitalWrite(MUX2_S0_PIN, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(MUX2_S1_PIN, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(MUX2_S2_PIN, (channel & 0x04) ? HIGH : LOW);
}

inline void sensorsBegin() {
  pinMode(MUX_SIG_PIN, INPUT);

  pinMode(MUX1_INH_PIN, OUTPUT);
  pinMode(MUX1_S0_PIN, OUTPUT);
  pinMode(MUX1_S1_PIN, OUTPUT);
  pinMode(MUX1_S2_PIN, OUTPUT);

  pinMode(MUX2_INH_PIN, OUTPUT);
  pinMode(MUX2_S0_PIN, OUTPUT);
  pinMode(MUX2_S1_PIN, OUTPUT);
  pinMode(MUX2_S2_PIN, OUTPUT);

  // Disable both muxes until a channel is selected.
  digitalWrite(MUX1_INH_PIN, HIGH);
  digitalWrite(MUX2_INH_PIN, HIGH);

#if USE_SENSOR_POWER_SWITCH
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, LOW);
#endif
}

inline void sensorsPowerOn() {
#if USE_SENSOR_POWER_SWITCH
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  delay(50);
#endif
}

inline void sensorsPowerOff() {
#if USE_SENSOR_POWER_SWITCH
  digitalWrite(SENSOR_POWER_PIN, LOW);
#endif
}

inline int readRawMoisture(uint8_t sensorIndex) {
  if (sensorIndex >= SENSOR_COUNT) {
    return 0;
  }

  const bool useMux2 = sensorIndex >= 8;
  const uint8_t channel = useMux2 ? (sensorIndex - 8) : sensorIndex;
  setMuxChannel(useMux2, channel);
  delay(3);

  long sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(MUX_SIG_PIN);
    delay(2);
  }

  // Disable both muxes between reads to reduce channel bleed.
  digitalWrite(MUX1_INH_PIN, HIGH);
  digitalWrite(MUX2_INH_PIN, HIGH);

  return (int)(sum / 8);
}

inline int readBatteryPercent() {
  const int raw = analogRead(BATTERY_ADC_PIN);
  const float vAdc = (raw / 4095.0f) * 3.3f;
  const float vBat = vAdc * BATTERY_DIVIDER_RATIO;
  const float pct = (vBat - BATTERY_V_MIN) * 100.0f / (BATTERY_V_MAX - BATTERY_V_MIN);
  return (int)constrain(pct, 0, 100);
}

inline void readAllRaw(int outRaw[SENSOR_COUNT]) {
  sensorsPowerOn();
  for (int i = 0; i < SENSOR_COUNT; i++) {
    outRaw[i] = readRawMoisture(i);
  }
  sensorsPowerOff();
}

inline void readAllMoisture(const CalibrationSettings& cal, int outPct[SENSOR_COUNT]) {
  int raw[SENSOR_COUNT];
  readAllRaw(raw);
  for (int i = 0; i < SENSOR_COUNT; i++) {
    outPct[i] = rawToPercentForZone(raw[i], cal, i);
  }
}
