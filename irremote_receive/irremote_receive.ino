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
      uint16_t type = (code & 0xFF000000) >> 24;
      uint16_t value = (code & 0x0000FFFF);

      switch (type) {
      case 2:
      case 3: {
        // Joy stick positions. Range: [0,255]
        uint8_t x_raw = (value & 0xFF00) >> 8;
        uint8_t y_raw = (value & 0x00FF) >> 0;

        Serial.print("Joystick ");
        Serial.print(type - 1);
        Serial.print(": ");
        Serial.print(x_raw);
        Serial.print(", ");
        Serial.print(y_raw);
        Serial.println();
        break;
      }
      case 1: {
        // Button
        uint16_t key = value;
        Serial.print("Key: ");
        Serial.println(key);
        break;
      }
      }
    }

    // Serial.print("Protocol: ");
    // Serial.print(protocol);
    // Serial.print(" Raw Code: ");
    // Serial.println(code);
    IrReceiver.resume();
  }
}
