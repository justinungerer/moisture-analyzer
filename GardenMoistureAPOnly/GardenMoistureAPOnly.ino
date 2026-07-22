#include <Arduino.h>
#include <WiFi.h>

const char* kApSsid = "Garden-Setup-TEST";
const char* kApPass = "garden1234";
const uint8_t kLedPin = 13;

void setup() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);

  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("AP-only test starting");

  WiFi.mode(WIFI_AP);
  delay(200);

  // Force a common 2.4 GHz channel and visible WPA2 AP for better compatibility.
  bool ok = WiFi.softAP(kApSsid, kApPass, 6, false, 4);
  if (ok) {
    Serial.println("AP started");
    Serial.print("SSID: ");
    Serial.println(kApSsid);
    Serial.print("PASS: ");
    Serial.println(kApPass);
    Serial.print("CH: ");
    Serial.println(WiFi.channel());
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("AP start failed");
  }
}

void loop() {
  static unsigned long lastMs = 0;
  if (millis() - lastMs >= 1000) {
    lastMs = millis();
    digitalWrite(kLedPin, !digitalRead(kLedPin));
    Serial.print("Stations connected: ");
    Serial.println(WiFi.softAPgetStationNum());
  }
}
