#include <Arduino.h>
#include "src/DYPlayerArduino.h"

DY::Player player(&Serial1);

int file_count = 10;
void setup() {
  player.begin();
  Serial.begin(115200);
  delay(400);
  file_count = player.getSoundCount();
  Serial.print("File count: ");
  Serial.println(file_count);
  player.setVolume(20);
  
}

void loop() {
  static int index = 1;
  DY::play_state_t state = player.checkPlayState();
  if (state == DY::play_state_t::Stopped) {
    Serial.print("Playing ");
    Serial.print(index);
    Serial.print("/");
    Serial.println(file_count);

    player.playSpecified(index);
    index++;
    if (index > file_count) {
      index = 1;
    }
  }
  delay(500);
}
