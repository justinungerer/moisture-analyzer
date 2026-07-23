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

  // Full 0–3.3 V ADC range with 12-bit resolution.
  analogReadResolution(12);
#if defined(ARDUINO_ARCH_ESP32)
  analogSetPinAttenuation(MUX_SIG_PIN, ADC_11db);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
#endif

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

// Simple insertion sort for the tiny sample buffers used below.
inline void sortInts(int* a, int n) {
  for (int i = 1; i < n; i++) {
    const int key = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > key) {
      a[j + 1] = a[j];
      j--;
    }
    a[j + 1] = key;
  }
}

// Trimmed mean: take SENSOR_SAMPLES readings, drop the SENSOR_DISCARD
// highest and lowest, average the rest. Rejects transient spikes/dropouts.
inline int readRawMoisture(uint8_t sensorIndex) {
  if (sensorIndex >= SENSOR_COUNT) {
    return 0;
  }

  const bool useMux2 = sensorIndex >= 8;
  const uint8_t channel = useMux2 ? (sensorIndex - 8) : sensorIndex;
  setMuxChannel(useMux2, channel);
  delay(3);

  int samples[SENSOR_SAMPLES];
  for (int i = 0; i < SENSOR_SAMPLES; i++) {
    samples[i] = analogRead(MUX_SIG_PIN);
    delay(2);
  }

  // Disable both muxes between reads to reduce channel bleed.
  digitalWrite(MUX1_INH_PIN, HIGH);
  digitalWrite(MUX2_INH_PIN, HIGH);

  sortInts(samples, SENSOR_SAMPLES);

  int lo = SENSOR_DISCARD;
  int hi = SENSOR_SAMPLES - SENSOR_DISCARD;
  if (hi <= lo) {  // guard against over-trimming a tiny buffer
    lo = 0;
    hi = SENSOR_SAMPLES;
  }

  long sum = 0;
  for (int i = lo; i < hi; i++) {
    sum += samples[i];
  }
  return (int)(sum / (hi - lo));
}

inline float readBatteryVoltage() {
  long sum = 0;
  for (int i = 0; i < BATTERY_SAMPLES; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
    delay(2);
  }
  const float raw  = (float)sum / BATTERY_SAMPLES;
  const float vAdc = (raw / 4095.0f) * 3.3f;
  return vAdc * BATTERY_DIVIDER_RATIO;
}

// Li-ion discharge is nonlinear; a piecewise curve tracks state-of-charge
// far better than a straight voltage→% line.
inline int batteryPercentFromVoltage(float v) {
  struct Point { float v; int pct; };
  static const Point curve[] = {
    {4.20f, 100}, {4.10f, 95}, {4.00f, 85}, {3.90f, 75}, {3.80f, 62},
    {3.70f, 48},  {3.60f, 32}, {3.50f, 18}, {3.40f, 9},  {3.30f, 4},
    {BATTERY_V_MIN, 0}
  };
  const int n = sizeof(curve) / sizeof(curve[0]);

  if (v >= curve[0].v) return 100;
  if (v <= curve[n - 1].v) return 0;

  for (int i = 0; i < n - 1; i++) {
    if (v <= curve[i].v && v > curve[i + 1].v) {
      const float span = curve[i].v - curve[i + 1].v;
      const float frac = (v - curve[i + 1].v) / span;
      const float pct  = curve[i + 1].pct + frac * (curve[i].pct - curve[i + 1].pct);
      return (int)constrain(pct, 0, 100);
    }
  }
  return 0;
}

inline int readBatteryPercent() {
  return batteryPercentFromVoltage(readBatteryVoltage());
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
