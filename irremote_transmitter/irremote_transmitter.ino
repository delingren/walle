#include <IRremote.hpp>
#include <Keypad.h>

class PushButton {
private:
  const int debounce_ = 50;
  int pin_;
  unsigned long last_ = 0;
  bool released_ = true;

public:
  PushButton(int pin) : pin_(pin) {}

  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    last_ = millis();
  }

  bool isPushed() {
    unsigned long now = millis();
    if (now - last_ < debounce_) {
      return false;
    }
    last_ = now;
    int state = digitalRead(pin_);
    // No hold and repeat. The button is only considered pushed
    // if it was released in the previous scan.
    if (state == LOW && released_) {
      released_ = false;
      return true;
    }
    if (state == HIGH) {
      released_ = true;
    }
    return false;
  }
};

class Joystick {
private:
  const int threshold = 2;

  int pin_x_;
  int pin_y_;

  uint8_t x_prev = 128;
  uint8_t y_prev = 128;

public:
  Joystick(int pin_x, int pin_y) : pin_x_(pin_x), pin_y_(pin_y) {}

  void begin() {
    pinMode(pin_x_, INPUT);
    pinMode(pin_y_, INPUT);
  }

  // Read x and y positions. Return true if either has shifted enough from the
  // last position.
  bool read(uint8_t &x, uint8_t &y) {
    // analogRead has 10 bit accuracy. We use 8 bits.
    x = analogRead(pin_x_) >> 2;
    y = 255 - (analogRead(pin_y_) >> 2); // Y axis is inverted.

    int delta_x = abs(x - x_prev);
    int delta_y = abs(y - y_prev);

    if (delta_x >= threshold || delta_y >= threshold) {
      x_prev = x;
      y_prev = y;
      return true;
    } else {
      return false;
    }
  }
};

class IrRemote {
private:
  int pin_;

  enum TYPE { KEY = 1, JOYSTICK1 = 2, JOYSTICK2 = 3 };

  void Send(uint16_t type, uint16_t value) {
    digitalWrite(LED_BUILTIN, HIGH);
    IrSender.sendNECRaw((uint32_t)type << 24 | value);
    delay(40);
    digitalWrite(LED_BUILTIN, LOW);
  }

public:
  IrRemote(int pin) : pin_(pin) {}

  begin() {
    IrSender.begin(pin_);
    pinMode(LED_BUILTIN, OUTPUT);
  }

  void SendKey(uint16_t key) {
    Serial.print("Key ");
    Serial.println(key);
    Send(TYPE::KEY, key);
  }

  void SendJoystick1(uint8_t x, uint8_t y) {
    Serial.print("Joystick 1: ");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.println();

    Send(TYPE::JOYSTICK1, (uint16_t)x << 8 | y);
  }

  void SendJoystick2(uint8_t x, uint8_t y) {
    Serial.print("Joystick 2: ");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.println();

    Send(TYPE::JOYSTICK2, (uint16_t)x << 8 | y);
  }
};

constexpr int pin_j1 = A0;
constexpr int pin_j2 = A1;
constexpr int pin_select = 11;
constexpr int pin_mode = 12;
constexpr int pin_start = A4;
constexpr int pin_ir = 10;

PushButton button_j1(pin_j1);
PushButton button_j2(pin_j2);
PushButton buttons[] = {PushButton(pin_j1), PushButton(pin_j2),
                        PushButton(pin_select), PushButton(pin_mode),
                        PushButton(pin_start)};

constexpr int rows = 2;
constexpr int cols = 6;
byte keys[rows][cols] = {{21, 22, 23, 24, 25, 26}, {27, 28, 29, 30, 31, 32}};
byte pins_rows[rows] = {2, 3};
byte pins_cols[cols] = {4, 5, 6, 7, 8, 9};

Keypad keypad = Keypad(makeKeymap(keys), pins_rows, pins_cols, rows, cols);

IrRemote remote(pin_ir);

Joystick joystick1(A3, A6);
Joystick joystick2(A2, A7);

void setup() {
  Serial.begin(9600);

  remote.begin();

  for (int i = 0; i < sizeof(buttons) / sizeof(PushButton); i++) {
    buttons[i].begin();
  }

  joystick1.begin();
  joystick2.begin();
}

void loop() {
  int key = keypad.getKey();
  if (key != 0) {
    remote.SendKey(key);
    return;
  }

  for (int i = 0; i < sizeof(buttons) / sizeof(PushButton); i++) {
    if (buttons[i].isPushed()) {
      remote.SendKey(i + 1);
      return;
    }
  }

  uint8_t x, y;
  if (joystick1.read(x, y)) {
    remote.SendJoystick1(x, y);
    return;
  }
  if (joystick2.read(x, y)) {
    remote.SendJoystick2(x, y);
    return;
  }
}