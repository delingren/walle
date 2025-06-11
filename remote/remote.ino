#define FEEDBACK_LED_IS_ACTIVE_LOW
#define DECODE_NEC
#include "src/IRremote.hpp"

#define NDEBUG
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
  byte *bitmap;

public:
  // No memory management for row and col pin arrays. Pass arrays on the heap.
  KeyMatrix(int rows, int cols, uint8_t *pinsRows, uint8_t *pinsCols)
      : rows_(rows), cols_(cols), pinsRows_(pinsRows), pinsCols_(pinsCols) {
    // (cols+1)>>3 == ceiling(cols/8)
    bitmap = (byte *)malloc(rows * ((cols + 1) >> 3));
  }

  void begin() {
    for (int c = 0; c < cols_; c++) {
      pinMode(pinsCols_[c], OUTPUT);
      digitalWrite(pinsCols_[c], HIGH);
    }
    for (int r = 0; r < rows_; r++) {
      pinMode(pinsRows_[r], INPUT_PULLUP);
    }
  }

  byte *read() {
    for (int c = 0; c < cols_; c++) {
      digitalWrite(pinsCols_[c], LOW);
      for (int r = 0; r < rows_; r++) {
        set(r, c, !digitalRead(pinsRows_[r]));
      }
      digitalWrite(pinsCols_[c], HIGH);
    }
    return bitmap;
  }

private:
  inline void set(int row, int col, bool value) {
    byte *target = &bitmap[(row * cols_ + col) >> 3];
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

public:
  Joystick(int pinHigh, int pinX, int pinY)
      : pinHigh_(pinHigh), pinX_(pinX), pinY_(pinY) {}

  void begin() {
    pinMode(pinHigh_, OUTPUT);
    pinMode(pinX_, INPUT);
    pinMode(pinY_, INPUT);

    digitalWrite(pinHigh_, HIGH);
    digitalWrite(pinHigh_, LOW);
  }

  byte read() {
    digitalWrite(pinHigh_, HIGH);
    float x = round(analogRead(pinX_) * (7.0 / 512.0) - 7.0);
    float y = round(7.0 - analogRead(pinY_) * (7.0 / 512.0));

    // IrSender doesn't like it when all bits are zero. So we use the
    // non-canonical representation of 0 b1000 to avoid all zero bits
    digitalWrite(pinHigh_, LOW);
    byte result = 0;
    result |= x <= 0 ? 0x08 : 0x00;
    result |= (uint8_t)(abs(x));
    result |= y <= 0 ? 0x80 : 0x00;
    result |= (uint8_t)(abs(y)) << 4;
    return result;
  }
};

class IrRemote {
private:
  uint8_t pin_;

public:
  IrRemote(uint8_t pin) : pin_(pin) {}

  void begin() { IrSender.begin(pin_); }

  void send(byte *data, int len) {
#ifdef NDEBUG
    Serial.print("Sending");
    char buffer[8];
    for (int i = 0; i < len; i++) {
      sprintf(buffer, " %02X", data[i]);
      Serial.print(buffer);
    }
    Serial.println();
#endif

    IrSender.sendPulseDistanceWidthFromArray(&NECProtocolConstants,
                                             (IRRawDataType *)data, len * 8, 0);
  }
};

IrRemote remote(10);

Joystick joystick1(A5, A2, A3);
Joystick joystick2(A5, A6, A7);

PushButton directButtons[] = {PushButton(A0), PushButton(A1)};

constexpr int rows = 2;
constexpr int cols = 8;
uint8_t pinsRows[rows] = {11, 12};
uint8_t pinsCols[cols] = {2, 3, 4, 5, 6, 7, 8, 9};
KeyMatrix matrix(rows, cols, pinsRows, pinsCols);

void setup() {
#ifdef NDEBUG
  Serial.begin(115200);
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
  if (now - last < 120) {
    return;
  }
  last = now;

  // Bytes 0-1: left and right joysticks
  // y3 y2 y1 y0 x3 x2 x1 x0
  // Bytes 2-3: matrix buttons
  // Byte 4: other buttons
  // Byte 5: xor of other bytes
  byte data[6];
  data[0] = joystick1.read();
  data[1] = joystick2.read();

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

  data[5] = data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4];

  // If the state hasn't been remaining neutral, we use an exponential
  // backoff strategy for resending the same neutral state.
  static unsigned long interval = 2;
  byte neutral = data[0] == 0x88 && data[1] == 0x88 && data[2] == 0 &&
                 data[3] == 0 && data[4] == 0;
  if (!neutral) {
    interval = 2;
    remote.send(data, sizeof(data) / sizeof(byte));
  } else {
    static unsigned long last = millis();
    if (now - last >= interval) {
      remote.send(data, sizeof(data) / sizeof(byte));
      last = now;
      interval = interval * 1.5;
    }
  }
}