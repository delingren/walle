
#define NO_LED_FEEDBACK_CODE
#define NO_DECODER
#include <IRremote.hpp>

#define NDEBUG

#ifdef NDEBUG
#define DEBUG_OUTPUT(...) Serial.print(__VA_ARGS__)
#else
#define DEBUG_OUTPUT(...)
#endif

/*
Bytes 0-1: joystick left & right
y3 y2 y1 y0 x3 x2 x1 x0
Bytes 2-4: buttons
*/

class IrRemote {
private:
  int pin_;

  enum TYPE { KEY = 1, JOYSTICK1 = 2, JOYSTICK2 = 3 };

  void Send(uint16_t type, uint16_t value) {
    IrSender.sendNECRaw((uint32_t)type << 24 | value);
  }

public:
  IrRemote(int pin) : pin_(pin) {}

  begin() { IrSender.begin(pin_); }

  void SendKey(uint16_t key) {
    DEBUG_OUTPUT("Key ");
    DEBUG_OUTPUT(key);
    DEBUG_OUTPUT('\n');
    // Send(TYPE::KEY, key);
  }

  void SendJoystick1(uint8_t x, uint8_t y) {
    DEBUG_OUTPUT("Joystick 1: ");
    DEBUG_OUTPUT(x);
    DEBUG_OUTPUT(", ");
    DEBUG_OUTPUT(y);
    DEBUG_OUTPUT('\n');

    Send(TYPE::JOYSTICK1, (uint16_t)x << 8 | y);
  }

  void SendJoystick2(uint8_t x, uint8_t y) {
    DEBUG_OUTPUT("Joystick 2: ");
    DEBUG_OUTPUT(x);
    DEBUG_OUTPUT(", ");
    DEBUG_OUTPUT(y);
    DEBUG_OUTPUT('\n');

    Send(TYPE::JOYSTICK2, (uint16_t)x << 8 | y);
  }
};

class Joystick {
private:
  const int sensitivity = 5;
  const int debounce_ = 50;

  unsigned long last_ = 0;
  int pinHigh_;
  int pinX_;
  int pinY_;

  uint8_t xPrev = 128;
  uint8_t yPrev = 128;

public:
  Joystick(int pinHigh, int pinX, int pinY)
      : pinHigh_(pinHigh), pinX_(pinX), pinY_(pinY) {}

  void begin() {
    pinMode(pinHigh_, OUTPUT);
    pinMode(pinX_, INPUT);
    pinMode(pinY_, INPUT);

    digitalWrite(pinHigh_, HIGH);
    xPrev = analogRead(pinX_) >> 2;
    yPrev = 255 - (analogRead(pinY_) >> 2); // Y axis is inverted.
    digitalWrite(pinHigh_, LOW);

    last_ = millis();
  }

  // Read x and y positions. Return true if either has shifted enough from the
  // last position.
  bool read(uint8_t &x, uint8_t &y) {
    unsigned long now = millis();
    if (now - last_ < debounce_) {
      return false;
    }
    last_ = now;

    digitalWrite(pinHigh_, HIGH);
    // analogRead has 10 bit accuracy. We use 8 bits.
    x = analogRead(pinX_) >> 2;
    y = 255 - (analogRead(pinY_) >> 2);
    digitalWrite(pinHigh_, LOW);

    int deltaX = abs(x - xPrev);
    int deltaY = abs(y - yPrev);

    if (deltaX >= sensitivity || deltaY >= sensitivity) {
      xPrev = x;
      yPrev = y;
      return true;
    } else {
      return false;
    }
  }
};

IrRemote remote(10);

Joystick joystick1(A4, A3, A2);
Joystick joystick2(A4, A1, A0);


void setup() {
  IrSender.begin(10);
}


void loop() {
  long start = millis();
  // Protocol: 2 Raw Code: 0xBA9876543210

  uint8_t buffer[6] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA};
  IrSender.sendPulseDistanceWidthFromArray(&NECProtocolConstants, (IRRawDataType*)buffer, 8 * sizeof(buffer)/sizeof(buffer[0]), 0);

  long end = millis();
  Serial.print("Time (ms): ");
  Serial.println(end - start);
  delay(500);
}
