#include <Servo.h>

Servo servo;

constexpr int min_us = 500;
constexpr int max_us = 2550;

void setup() {
  servo.attach(8, min_us, max_us);
  Serial.begin(115200);
}

void loop() {
  static int step_us = (max_us - min_us) / 100;
  static int us = min_us;

  servo.writeMicroseconds(us);
  Serial.println(us);
  delay(20);
  us += step_us;
  if (us >= max_us) {
    us = max_us;
    step_us *= -1;
  }
  if (us <= min_us) {
    us = min_us;
    step_us *= -1;
  }
}
