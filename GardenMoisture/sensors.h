#pragma once

#include "config.h"
#include "calibration.h"
#include <Arduino.h>

#if SENSOR_POWER_ACTIVE_LOW
  static constexpr uint8_t SENSOR_PWR_ON_LEVEL  = LOW;
  static constexpr uint8_t SENSOR_PWR_OFF_LEVEL = HIGH;
#else
  static constexpr uint8_t SENSOR_PWR_ON_LEVEL  = HIGH;
  static constexpr uint8_t SENSOR_PWR_OFF_LEVEL = LOW;
#endif

#if USE_SENSOR_POWER_SWITCH
inline bool gSensorPowerSwitchingEnabled = DEFAULT_SENSOR_POWER_SWITCHING_ENABLED;
#endif

inline void setMuxChannel(uint8_t channel) {
  channel &= 0x0F;

  digitalWrite(MUX_S0_PIN, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(MUX_S1_PIN, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(MUX_S2_PIN, (channel & 0x04) ? HIGH : LOW);
  digitalWrite(MUX_S3_PIN, (channel & 0x08) ? HIGH : LOW);

  // Active-low enable: pull LOW to connect the selected channel to SIG.
  digitalWrite(MUX_EN_PIN, LOW);
}

inline void sensorsBegin() {
  pinMode(MUX_SIG_PIN, INPUT);

  pinMode(MUX_EN_PIN, OUTPUT);
  pinMode(MUX_S0_PIN, OUTPUT);
  pinMode(MUX_S1_PIN, OUTPUT);
  pinMode(MUX_S2_PIN, OUTPUT);
  pinMode(MUX_S3_PIN, OUTPUT);

  // Disable the mux (active-low enable HIGH) until a channel is selected.
  digitalWrite(MUX_EN_PIN, HIGH);

  // Full 0–3.3 V ADC range with 12-bit resolution.
  analogReadResolution(12);
#if defined(ARDUINO_ARCH_ESP32)
  analogSetPinAttenuation(MUX_SIG_PIN, ADC_11db);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
#endif

#if USE_SENSOR_POWER_SWITCH
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, SENSOR_PWR_OFF_LEVEL);
#endif
}

inline void sensorsPowerOn() {
#if USE_SENSOR_POWER_SWITCH
  if (!gSensorPowerSwitchingEnabled) {
    return;
  }
  digitalWrite(SENSOR_POWER_PIN, SENSOR_PWR_ON_LEVEL);
  delay(SENSOR_POWER_ON_DELAY_MS);
#endif
}

inline void sensorsPowerOff() {
#if USE_SENSOR_POWER_SWITCH
  if (!gSensorPowerSwitchingEnabled) {
    return;
  }
  digitalWrite(SENSOR_POWER_PIN, SENSOR_PWR_OFF_LEVEL);
#endif
}

inline bool sensorsPowerSwitchingEnabled() {
#if USE_SENSOR_POWER_SWITCH
  return gSensorPowerSwitchingEnabled;
#else
  return false;
#endif
}

inline void sensorsSetPowerSwitching(bool enabled) {
#if USE_SENSOR_POWER_SWITCH
  gSensorPowerSwitchingEnabled = enabled;
  // When switching is disabled, force the rail ON continuously.
  digitalWrite(SENSOR_POWER_PIN, gSensorPowerSwitchingEnabled ? SENSOR_PWR_OFF_LEVEL : SENSOR_PWR_ON_LEVEL);
  if (!gSensorPowerSwitchingEnabled && SENSOR_POWER_ON_DELAY_MS > 0) {
    delay(SENSOR_POWER_ON_DELAY_MS);
  }
#else
  (void)enabled;
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

  // sensorIndex (0-14) maps directly to a CD74HC4067 channel.
  setMuxChannel(sensorIndex);
  delay(3);

  int samples[SENSOR_SAMPLES];
  for (int i = 0; i < SENSOR_SAMPLES; i++) {
    samples[i] = analogRead(MUX_SIG_PIN);
    delay(2);
  }

  // Disable the mux between reads to reduce channel bleed.
  digitalWrite(MUX_EN_PIN, HIGH);

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
  // Keep battery reads independent from the switched sensor rail.
  sensorsPowerOff();
  if (BATTERY_ADC_SETTLE_MS > 0) {
    delay(BATTERY_ADC_SETTLE_MS);
  }

  // On ESP32 ADC, a throwaway read helps charge the sample capacitor.
  (void)analogRead(BATTERY_ADC_PIN);
  const float raw  = (float)analogRead(BATTERY_ADC_PIN);
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
