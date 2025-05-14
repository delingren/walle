#include <IRremote.hpp>

const int pin_ir_receiver = 10;

void setup() {
  Serial.begin(115200);
  IrSender.begin(pin_ir_receiver);
}

void loop() {
  IrSender.sendNECRaw(0x12345678);
  delay(1000);
}
