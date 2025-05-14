#include <IRremote.hpp>

const int pin_error = 8;
const int pin_alive = 28;
const int pin_ir_led = 3;

bool alive = false;
bool error = false;

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(pin_ir_led, false);

  pinMode(pin_alive, OUTPUT);
  pinMode(pin_error, OUTPUT);
}

void loop() {
  static long last = millis();
  long now = millis();

  if (IrReceiver.decode()) {
    last = now;
    uint32_t code = IrReceiver.decodedIRData.decodedRawData;
    decode_type_t protocol = IrReceiver.decodedIRData.protocol;
    error = !(protocol == 10 && code == 0x12345678);

    IrReceiver.resume();
  }

  alive = (now - last) < 600;

  digitalWrite(pin_error, error);
  digitalWrite(pin_alive, alive);
}
