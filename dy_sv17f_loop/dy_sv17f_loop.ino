#include <Arduino.h>
#include "src/DYPlayerArduino.h"

DY::Player player(&Serial1);

int file_count = 10;
void setup() {
  player.begin();
  delay(200);
  player.setVolume(8);
}

void loop() {
  static int index = 1;
  DY::play_state_t state = player.checkPlayState();
  if (((int) state) == 0) {
    Serial.println(index);
    player.playSpecified(index);
    index++;
    if (index > file_count) {
      index = 1;
    }
  }
  delay(500);
}
