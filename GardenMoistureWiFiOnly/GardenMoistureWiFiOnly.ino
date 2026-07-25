#include <Arduino.h>
#include <WiFi.h>

const char* kSsid = "Why Fie?_2EXT";
const char* kPassword = "peguinssuck";
const uint8_t kLedPin = 13;

void blink(int count, int delayMs) {
  for (int i = 0; i < count; ++i) {
    digitalWrite(kLedPin, HIGH);
    delay(delayMs);
    digitalWrite(kLedPin, LOW);
    delay(delayMs);
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(500);

  Serial.print("Connecting to ");
  Serial.println(kSsid);
  WiFi.begin(kSsid, kPassword);

  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 20000) {
    Serial.print('.');
    digitalWrite(kLedPin, !digitalRead(kLedPin));
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(kLedPin, HIGH);
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    digitalWrite(kLedPin, LOW);
    Serial.println("WiFi connect failed");
    Serial.print("Status: ");
    Serial.println((int)WiFi.status());
  }
}

void setup() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);

  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("GardenMoisture WiFi-only test");
  connectWiFi();
}

void loop() {
  static unsigned long lastStatusMs = 0;
  if (millis() - lastStatusMs >= 5000) {
    lastStatusMs = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Still connected, IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Disconnected, retrying...");
      connectWiFi();
    }
  }
}
