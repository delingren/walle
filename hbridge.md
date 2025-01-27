# Driving motors with H-bridges

The main motors need to be able to rotate both directions and the speed needs to be controlled. So we'll use two H-bridges. I found this IC L293D that has two full H-bridges and can drive 600mA. It sounds exactly what I need. And here is a [good tutorial](https://lastminuteengineers.com/l293d-dc-motor-arduino-tutorial/) on this IC and H-bridges in general.

So I loaded a [sketch](./motor_hbridge/), as follows, to test out controlling the speed and direction of two motors.

```
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
```

![h-bridge](./media/IMG_0817.mov)