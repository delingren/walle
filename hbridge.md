# Driving motors with H-bridges

The main motors need to be able to rotate both directions and the speed needs to be controlled. So we'll use two H-bridges. In v2, I used an IC L293D that has two full H-bridges and can drive 600mA. However, once integrated, I found that the motors were not providing enough torque. I had to reduce the gear ratio and used two 75RPM DC motors. This resulted the tread moving more slowly than I wanted. I did a some trouble shooting and concluded that the two main reasons were the voltage drop on the H-bridge and lack of lubrication in the system. The second was easy to solve with some bike chain lube. For the former one, I decided to look for a different H-bridge IC based on MOSFET instead of BJT. So I came across [this post](https://dronebotworkshop.com/tb6612fng-h-bridge/) that introduces this TB6612FNG breakout board. It uses 6 GPIO pins rather than 4, 3 for each motor. Two pins control the direction and a third one controls the speed via PWM.

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