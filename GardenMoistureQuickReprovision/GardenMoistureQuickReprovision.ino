#include <Arduino.h>
#include <Preferences.h>

namespace {
const char* kNamespace = "blynk";
const uint8_t kLedPin = 13;
}

void setup() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);

  Preferences prefs;
  bool cleared = false;
  if (prefs.begin(kNamespace, false)) {
    cleared = prefs.clear();
    prefs.end();
  }

  for (int i = 0; i < (cleared ? 6 : 2); ++i) {
    digitalWrite(kLedPin, HIGH);
    delay(120);
    digitalWrite(kLedPin, LOW);
    delay(120);
  }
}

void loop() {
}
