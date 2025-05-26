#define NO_LED_FEEDBACK_CODE
#define DECODE_NEC
#include <IRremote.hpp>
#include <Keypad.h>

#define NDEBUG 1
// #undef NDEBUG

#ifdef NDEBUG
#define DEBUG_OUTPUT(...) Serial.print(__VA_ARGS__)
#else
#define DEBUG_OUTPUT(...)
#endif

class KeyMatrix {
private:
  int rows_, cols_;
  uint8_t *pinsRows_, *pinsCols_;
  byte *state;

public:
  // No memory management for row and col pin arrays. Pass arrays on the heap.
  KeyMatrix(int rows, int cols, uint8_t *pinsRows, uint8_t *pinsCols)
      : rows_(rows), cols_(cols), pinsRows_(pinsRows), pinsCols_(pinsCols) {
    state = (byte *)malloc(rows *
                           ((cols + 1) >> 3)); // (cols+1)>>3 == ceiling(cols/8)
  }

  void begin() {
    for (int r = 0; r < rows_; r++) {
      pinMode(pinsRows_[r], OUTPUT);
      digitalWrite(pinsRows_[r], HIGH);
    }
    for (int c = 0; c < cols_; c++) {
      pinMode(pinsCols_[c], INPUT_PULLUP);
    }
  }

  byte *read() {
    for (int r = 0; r < rows_; r++) {
      digitalWrite(pinsRows_[r], LOW);
      for (int c = 0; c < cols_; c++) {
        setState(r, c, !digitalRead(pinsCols_[c]));
      }
      digitalWrite(pinsRows_[r], HIGH);
    }
    return state;
  }

private:
  inline void setState(int row, int col, bool value) {
    byte *target = &state[(row * cols_ + col) >> 3];
    int bit = col & 0x07;
    if (value) {
      *target |= 1 << bit;
    } else {
      *target &= ~(1 << bit);
    }
  }
};

class PushButton {
private:
  int pin_;

public:
  PushButton(int pin) : pin_(pin) {}

  void begin() { pinMode(pin_, INPUT_PULLUP); }

  inline bool read() { return !digitalRead(pin_); }
};

class Joystick {
private:
  int pinHigh_;
  int pinX_;
  int pinY_;

  uint8_t xPrev;
  uint8_t yPrev;

public:
  Joystick(int pinHigh, int pinX, int pinY)
      : pinHigh_(pinHigh), pinX_(pinX), pinY_(pinY) {}

  void begin() {
    pinMode(pinHigh_, OUTPUT);
    pinMode(pinX_, INPUT);
    pinMode(pinY_, INPUT);

    digitalWrite(pinHigh_, HIGH);
    read(xPrev, yPrev);
    digitalWrite(pinHigh_, LOW);
  }

  void read(uint8_t &x, uint8_t &y) {
    digitalWrite(pinHigh_, HIGH);
    x = analogRead(pinX_) >> 6;
    y = (1023 - analogRead(pinY_) >> 6);
    digitalWrite(pinHigh_, LOW);
  }
};

class IrRemote {
private:
  int pin_;

public:
  IrRemote(int pin) : pin_(pin) {}

  void begin() { IrSender.begin(pin_); }

  void send(byte *buffer, int len) {
    IrSender.sendPulseDistanceWidthFromArray(&NECProtocolConstants,
                                             (IRRawDataType *)buffer, 40, 0);
  }
};

IrRemote remote(10);

Joystick joystick1(A4, A2, A3);
Joystick joystick2(A5, A6, A7);

PushButton directButtons[] = {PushButton(A0), PushButton(A1)};

constexpr int rows = 2;
constexpr int cols = 8;
uint8_t pinsRows[rows] = {11, 12};
uint8_t pinsCols[cols] = {2, 3, 4, 5, 6, 7, 8, 9};
KeyMatrix matrix(rows, cols, pinsRows, pinsCols);

void setup() {
#ifdef NDEBUG
  Serial.begin(9600);
#endif

  remote.begin();

  for (int i = 0; i < sizeof(directButtons) / sizeof(PushButton); i++) {
    directButtons[i].begin();
  }

  joystick1.begin();
  joystick2.begin();
  matrix.begin();
}

void loop() {
  static unsigned long last = millis();
  unsigned long now = millis();
  // IR receiver needs about 100 ms to receive and process a frame.
  if (now - last < 110) {
    return;
  }
  last = now;

  /*
  Bytes 0-1: left and right joysticks
  y3 y2 y1 y0 x3 x2 x1 x0
  Bytes 2-3: matrix buttons
  Byte 4: other buttons
  */
  static byte prevData[5];

  byte data[5];
  uint8_t x, y;
  joystick1.read(x, y);
  data[0] = x | (y << 4);
  joystick2.read(x, y);
  data[1] = x | (y << 4);

  byte *b = matrix.read();
  data[2] = b[0];
  data[3] = b[1];

  data[4] = 0;
  for (int i = 0; i < sizeof(directButtons) / sizeof(PushButton); i++) {
    if (directButtons[i].read()) {
      data[4] |= 1 << i;
    } else {
      data[4] &= ~(1 << i);
    }
  }

  // Update prevData and determine if the new data is different
  byte changed = false;
  for (int i = 0; i < 5; i++) {
    changed |= data[i] ^ prevData[i];
    prevData[i] = data[i];
  }

  // If the state hasn't changed since last send, we use a exponential backoff
  // strategy for resending the same state.
  static unsigned long interval = 2;
  if (changed) {
    interval = 2;
    remote.send(data, sizeof(data) / sizeof(byte));
  } else {
    static unsigned long lastUnchanged = millis();

    if (now - lastUnchanged >= interval) {
      Serial.println("sending (unchanged)");
      remote.send(data, sizeof(data) / sizeof(byte));
      lastUnchanged = now;
      interval = interval * 1.5;
    }
  }
}