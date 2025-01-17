#include <Servo.h>

Servo servo;

int min_us = 0;
int max_us = 5000;

void setup() {
  Serial.begin();
  servo.attach(8, min_us, max_us);
  servo.writeMicroseconds(1500);
}

void loop() {
  while (!Serial.available()) {}
  int us = Serial.parseInt();
  Serial.println(us);
  servo.writeMicroseconds(us);
}
