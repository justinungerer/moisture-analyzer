void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("BOOT TEST: Serial OK");
  pinMode(2, OUTPUT); // common built-in LED on many ESP32 boards
}

void loop() {
  Serial.println("loop");
  digitalWrite(2, HIGH);
  delay(200);
  digitalWrite(2, LOW);
  delay(800);
}
