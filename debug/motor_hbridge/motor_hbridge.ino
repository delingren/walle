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
  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(in1A, OUTPUT);
  pinMode(in2A, OUTPUT);
  pinMode(in1B, OUTPUT);
  pinMode(in2B, OUTPUT);

  Serial.begin(115200);
  delay(1500);
}

void loop() {
  // Forward
  digitalWrite(in1A, HIGH);
  digitalWrite(in2A, LOW);
  digitalWrite(in1B, HIGH);
  digitalWrite(in2B, LOW);
  // Full speed
  analogWrite(pwmA, 255);
  analogWrite(pwmB, 255);
  delay(1500);
  // Half speed
  analogWrite(pwmA, 192);
  analogWrite(pwmB, 192);
  delay(1500);
  // Stop
  analogWrite(pwmA, 0);
  analogWrite(pwmB, 0);
  delay(1500);

  // Left spin
  digitalWrite(in1A, LOW);
  digitalWrite(in2A, HIGH);
  digitalWrite(in1B, HIGH);
  digitalWrite(in2B, LOW);
  // Full speed
  analogWrite(pwmA, 255);
  analogWrite(pwmB, 255);
  delay(1500);
  // Half speed
  analogWrite(pwmA, 192);
  analogWrite(pwmB, 192);
  delay(1500);
  // Stop
  analogWrite(pwmA, 0);
  analogWrite(pwmB, 0);
  delay(1500);

  // Right spin
  digitalWrite(in1A, HIGH);
  digitalWrite(in2A, LOW);
  digitalWrite(in1B, LOW);
  digitalWrite(in2B, HIGH);
  // Full speed
  analogWrite(pwmA, 255);
  analogWrite(pwmB, 255);
  delay(1500);
  // Half speed
  analogWrite(pwmA, 192);
  analogWrite(pwmB, 192);
  delay(1500);
  // Stop
  analogWrite(pwmA, 0);
  analogWrite(pwmB, 0);
  delay(1500);

  // Backward
  digitalWrite(in1A, LOW);
  digitalWrite(in2A, HIGH);
  digitalWrite(in1B, LOW);
  digitalWrite(in2B, HIGH);
  // Full speed
  analogWrite(pwmA, 255);
  analogWrite(pwmB, 255);
  delay(1500);
  // Half speed
  analogWrite(pwmA, 192);
  analogWrite(pwmB, 192);
  delay(1500);

  // Stop
  analogWrite(pwmA, 0);
  analogWrite(pwmB, 0);
  delay(1500);
}