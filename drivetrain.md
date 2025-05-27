# Driving motors with H-bridges

The main motors need to be able to rotate both directions and the speed needs to be controlled. So we'll use two H-bridges. In v2, I used an IC L293D that has two full H-bridges and can drive 600mA. However, once integrated, I found that the motors were not providing enough torque. I had to reduce the gear ratio and used two 75RPM DC motors. This resulted the tread moving more slowly than I wanted. I did a some trouble shooting and concluded that the two main reasons were the voltage drop on the H-bridge and lack of lubrication in the system. The second was easy to solve with some bike chain lube. For the former one, I decided to look for a different H-bridge IC based on MOSFET instead of BJT. So I came across [this post](https://dronebotworkshop.com/tb6612fng-h-bridge/) that introduces this TB6612FNG breakout board. It uses 6 GPIO pins rather than 4, 3 for each motor. Two pins control the direction and a third one controls the speed via PWM.

So I loaded a [sketch](./debug/motor_hbridge/), as follows, to test controlling the speed and direction of two motors.

```
// Motor A
int pwmA = 14;
int in1A = 12;
int in2A = 13;

// Motor B
int pwmB = 9;
int in1B = 11;
int in2B = 10;

int speed = 255;

void setup() {

  // Set all the motor control pins to outputs

  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(in1A, OUTPUT);
  pinMode(in2A, OUTPUT);
  pinMode(in1B, OUTPUT);
  pinMode(in2B, OUTPUT);

  Serial.begin(115200);
}

void loop() {

  // Set Motor A forward

  digitalWrite(in1A, HIGH);
  digitalWrite(in2A, LOW);

  // Set Motor B backward

  digitalWrite(in1B, LOW);
  digitalWrite(in2B, HIGH);

  analogWrite(pwmA, speed);
  analogWrite(pwmB, speed);

  Serial.println(speed);

  speed -= 50;

  if (speed <= 105) {
    speed = 255;
  }
  delay(3000);
}
```
