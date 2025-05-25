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