#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

namespace {
const char* kBlynkNamespace = "blynk";
const uint8_t kLedPin = 13;

void blinkStatus(int count, int delayMs) {
  for (int i = 0; i < count; ++i) {
    digitalWrite(kLedPin, HIGH);
    delay(delayMs);
    digitalWrite(kLedPin, LOW);
    delay(delayMs);
  }
}
}

void setup() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);

  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("GardenMoisture reprovision utility"));
  Serial.println(F("Clearing saved Blynk provisioning..."));

  Preferences prefs;
  bool cleared = false;
  if (prefs.begin(kBlynkNamespace, false)) {
    cleared = prefs.clear();
    prefs.end();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);

  if (cleared) {
    Serial.println(F("Blynk provisioning cleared."));
    blinkStatus(6, 150);
  } else {
    Serial.println(F("Blynk namespace could not be opened."));
    blinkStatus(2, 400);
  }

  Serial.println(F("Next step: re-upload GardenMoisture and provision it again in Blynk."));
}

void loop() {
  static unsigned long lastMessageMs = 0;
  if (millis() - lastMessageMs >= 3000) {
    lastMessageMs = millis();
    digitalWrite(kLedPin, !digitalRead(kLedPin));
    Serial.println(F("Provisioning is cleared. Re-upload GardenMoisture."));
  }
}
