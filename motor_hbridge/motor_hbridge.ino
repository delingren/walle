int left1 = 9;
int left2 = 10;
int right1 = 12;
int right2 = 13;

void setup() {
  pinMode(left1, OUTPUT);
  pinMode(left2, OUTPUT);
  pinMode(right1, OUTPUT);
  pinMode(right2, OUTPUT);
}

void loop() {
  static int speed = 0;
  static int inc = 1;

  if (speed >= 0) {
    analogWrite(left1, speed);
    analogWrite(left2, 0);

    analogWrite(right1, 0);
    analogWrite(right2, speed);
  } else {
    analogWrite(left1, 0);
    analogWrite(left2, -speed);
    
    analogWrite(right1, -speed);
    analogWrite(right2, 0);
  }

  speed += inc;
  if (speed >= 255 || speed <= -255) {
    inc = - inc;
  }
  
  delay(10);
}
