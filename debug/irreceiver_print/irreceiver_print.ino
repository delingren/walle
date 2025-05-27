#include <IRremote.hpp>

const int pin_ir_led = 3;

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(pin_ir_led, false);
}

void loop() {
  if (IrReceiver.decode()) {
    uint64_t code = IrReceiver.decodedIRData.decodedRawData;
    decode_type_t protocol = IrReceiver.decodedIRData.protocol;

    // Serial.print("Protocol: ");
    // Serial.print(protocol);
    // Serial.print(" Raw Code: 0x");
    // Serial.println(code, HEX);

    if (protocol == PULSE_DISTANCE) {
      byte *p = (byte *)(&code);
      byte correction = 0;
      for (int i = 0; i < 6; i++) {
        correction ^= p[i];
      }
      if (correction == 0) {
        static uint64_t prevCode = 0x000000008888; // The neutral state

        int8_t x1, y1;
        int8_t x2, y2;
        decodeJoystick(p[0], x1, y1);
        decodeJoystick(p[1], x2, y2);

        // if any of the button bits (16 - 40) has changed
        if ((code ^ prevCode) & 0xFFFFFF0000) {
          for (int i = 16; i < 40; i++) {
            bool curr = (code >> i) & 1;
            bool prev = (prevCode >> i) & 1;

            if (!curr && prev) {
              Serial.print("Button ");
              Serial.print(i - 16);
              Serial.println(" released.");
            }

            if (curr && !prev) {
              Serial.print("Button ");
              Serial.print(i - 16);
              Serial.println(" pressed.");
            }
          }
        }

        // if joystick 1 changed
        if ((code ^ prevCode) & 0x00000000FF) {
          Serial.print("Joystick 1: ");
          Serial.print(x1);
          Serial.print(' ');
          Serial.print(y1);
          Serial.print(' ');
          Serial.println();
        }

        // if joystick 2 changed
        if ((code ^ prevCode) & 0x000000FF00) {
          Serial.print("Joystick 2: ");
          Serial.print(x2);
          Serial.print(' ');
          Serial.print(y2);
          Serial.print(' ');
          Serial.println();
        }

        prevCode = code;
      } else {
        Serial.println("Error detected");
      }
    }

    IrReceiver.resume();
  }
}

void decodeJoystick(byte code, int8_t &x, int8_t &y) {
  if (code & 0x08) {
    x = -(code & 0x07);
  } else {
    x = code & 0x07;
  }

  if (code & 0x80) {
    y = -((code & 0x70) >> 4);
  } else {
    y = (code & 0x70) >> 4;
  }
}