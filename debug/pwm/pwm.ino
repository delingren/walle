int pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 26, 27, 28, 29};

void setup() {
  for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i ++) {
    pinMode(pins[i], OUTPUT);
  }
}

void loop() {
  static int brightness = 0;
  static int fadeAmount = 5;
  
  for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i ++) {
    analogWrite(pins[i], brightness);
  }

  brightness += fadeAmount;

  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }

  delay(30);
}
