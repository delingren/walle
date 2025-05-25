#define DECODE_NEC
#include <IRremote.hpp>

const int pin_ir_led = 3;

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(pin_ir_led, false);
}

void loop() {
  if (IrReceiver.decode()) {
    uint32_t code = IrReceiver.decodedIRData.decodedRawData;
    decode_type_t protocol = IrReceiver.decodedIRData.protocol;

    Serial.print("Protocol: ");
    Serial.print(protocol);
    Serial.print(" Raw Code: 0x");
    Serial.println(code, HEX);
    
    IrReceiver.resume();
  }
}
