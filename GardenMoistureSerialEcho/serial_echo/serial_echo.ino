void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Serial echo test ready. Type something and press Enter.");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    Serial.print("Received: ");
    Serial.println(line);
  }
}
