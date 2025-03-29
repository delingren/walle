#include <IRremote.hpp>

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(3, false);
}

void loop() {
  if (IrReceiver.decode()) {
    uint32_t code = IrReceiver.decodedIRData.decodedRawData;
    decode_type_t protocol = IrReceiver.decodedIRData.protocol;

    if (protocol == NEC2 || protocol == ONKYO) {
      uint16_t type = (code & 0xFFFF0000) >> 16;
      uint16_t value = (code & 0x0000FFFF);

      switch (type) {
        case 1: {
          // Joy stick positions. Range: [0,255]
          uint8_t x_raw = (value & 0xFF00) >> 8;
          uint8_t y_raw = (value & 0x00FF) >> 0;

          // Normalize x and y values. Range: [-1,1]
          float x_normalized = (x_raw - 127.5) / 127.5;
          float y_normalized = (y_raw - 127.5) / 127.5;

          float x_sign = x_normalized >= 0 ? 1 : -1;
          float y_sign = y_normalized >= 0 ? 1 : -1;

          float left, right;
          if (x_sign == y_sign) {
            left = y_sign * max(abs(x_normalized), abs(y_normalized));
            right = y_sign * (abs(y_normalized) - abs(x_normalized));
          } else {
            right = y_sign * max(abs(x_normalized), abs(y_normalized));
            left = y_sign * (abs(y_normalized) - abs(x_normalized));
          }

          Serial.println(left);
          Serial.println(right);
          break;
        }
        case 2: {
          // Button
          uint16_t key = value;
          Serial.print("Key: ");
          Serial.println(key);
        }

        break;
      }
    }

    Serial.print("Protocol: ");
    Serial.println(protocol);
    Serial.print("Code: ");
    Serial.println(code);
    IrReceiver.resume();
  }
}
