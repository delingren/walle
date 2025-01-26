# Audio

I used a [DFPlayer](https://www.dfrobot.com/product-1121.html) in the first version and thought it was a bit expensive (~$20 at the time). But the price seems to have come down quite a bit. AliExpress even has some clones that costs only $1 a piece. It doesn't have internal flash. Instead, it reads from microSD cards. I have quite a few of microSD cards salvaged from all sorts of electronics found in e-waste bins. There are some boards out there that come with internal storage. But since this one works fine and is rather cheap, I didn't bother with alternatives.

So, let's see if it plays nicely with the RP2040-Zero. The board comes with an Arduino [library](https://github.com/DFRobot/DFRobotDFPlayerMini). The wiring is rather simple. You just need to provide it with power and connect it with a UART port, which I'm using `Serial1` on Pi Pico (pins 0 and 1 by default), and connect the speaker to the output pins. There isn't much to it. Then I loaded a sketch to play all 10 audio files on the SD card.

```
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
```

And voila!

![audio](./media/IMG_1112.mov)

A couple of caveats to mention:

1. For some reason, reading file count is not very reliable on this board. I think it's this particular board, since I didn't have any issues in the paste. No big deal, I just need to hard code it.
1. The board takes 5V DC but I am going to power everything with 4xAA batteries and I hope to get away without using a regulator. It seems to be able to tolerate 6V. But it's safer to put a 1N1007 diode in serial to drop 0.7V.