void setup() {
  pinMode(14, OUTPUT);
}

void loop() {
  digitalWrite(14, HIGH);
  delay(3000);
  digitalWrite(14, LOW);
  delay(1000);
}
