#include <DFRobotDFPlayerMini.h>

DFRobotDFPlayerMini audioPlayer;
int file_count = 10;

void setup() {
  Serial.begin(115200);

  // Set up audio player over serial 1
  Serial1.begin(9600);
  delay(200);
  if (!audioPlayer.begin(Serial1)) {
    Serial.println("Failed to initialize audio player.");
  } else {
    Serial.println("Successfully initialized audio player.");
  }
  audioPlayer.volume(5);  // Volume: 0 - 30

  // For some reason, reading file count is not reliable on this particular board. So I'm hard coding it.
  // file_count = audioPlayer.readFileCounts();
}

void loop() {
  // File index starts with 1.
  static int index = 1;

  Serial.print("Playing: ");
  Serial.println(index);

  audioPlayer.play(index);
  delay(4000);

  index++;
  if (index > file_count) {
    index = 1;
  }
}
