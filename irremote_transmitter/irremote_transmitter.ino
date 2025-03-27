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

class Joystick {};

constexpr int pin_j1 = A0;
constexpr int pin_j2 = A1;
constexpr int pin_select = 11;
constexpr int pin_mode = 12;
constexpr int pin_start = 13;

PushButton button_j1(pin_j1);
PushButton button_j2(pin_j2);
PushButton buttons[] = {PushButton(pin_j1), PushButton(pin_j2),
                        PushButton(pin_select), PushButton(pin_mode),
                        PushButton(pin_start)};

constexpr int rows = 2;
constexpr int cols = 6;
byte keys[rows][cols] = {{1, 2, 3, 4, 5, 6}, {7, 8, 9, 10, 11, 12}};
byte pins_rows[rows] = {2, 3};
byte pins_cols[cols] = {4, 5, 6, 7, 8, 9};

Keypad keypad = Keypad(makeKeymap(keys), pins_rows, pins_cols, rows, cols);

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < sizeof(buttons) / sizeof(PushButton); i++) {
    buttons[i].begin();
  }
}

void loop() {
  // Read keypad
  int key = keypad.getKey();
  if (key != 0) {
    Serial.print("keypad: ");
    Serial.println(key);
  }

  for (int i = 0; i < sizeof(buttons) / sizeof(PushButton); i++) {
    if (buttons[i].isPushed()) {
      Serial.print(i);
      Serial.println(" pushed.");
    }
  }
}