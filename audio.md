# Audio

## DFPlayer

I used a [DFPlayer](https://www.dfrobot.com/product-1121.html) in the first version and thought it was a bit expensive (~$20 at the time). But the price has come down quite a bit. AliExpress even has some clones that costs just a little over $1 a piece. It doesn't have internal flash. Instead, it reads from microSD cards. I have quite a few of microSD cards salvaged from all sorts of electronics found in e-waste bins.

So, let's see if it plays nicely with the RP2040-Zero. The board comes with an Arduino [library](https://github.com/DFRobot/DFRobotDFPlayerMini). The wiring is rather simple. You just need to provide it with power and connect it with a UART port, which I'm using `Serial1` on Pi Pico (pins 0 and 1 by default for Tx and Rx respectively), and connect the speaker to the output pins. There isn't much to it. Then I loaded a [sketch](./dfplayer_loop/) to play all 10 audio files on the SD card in a loop.

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

## DY-SV17F

I also found an even cheaper alternative: DY-SV17F which has 4MB internal flash and an 5W amp. It has some [official documentation](https://github.com/smoluks/DY-SV17F) but no official Arduino library. I found a third party [library](https://github.com/SnijderC/dyplayer) and decided to give it a try with the following [sketch](./dy_sv17f_loop/), similar to the previous one.

```
#include <Arduino.h>
#include "src/DYPlayerArduino.h"

DY::Player player(&Serial1);

int file_count = 10;
void setup() {
  player.begin();
  delay(200);
  player.setVolume(15);
}

void loop() {
  static int index = 1;
  DY::play_state_t state = player.checkPlayState();
  if (state == DY::play_state_t::Stopped) {
    player.playSpecified(index);
    index++;
    if (index > file_count) {
      index = 1;
    }
  }
  delay(500);
}
```

It seems to work fine. So I may go with this one for its low cost and simplicity, since it requires no SD card. A few things worth mentioning:

1. It requires three 10K Ohm resistors to configure it to UART mode.
1. It comes with a micro USB port for accessing its filesystem on the flash. It might be an isolated incident but when I copied files from a Mac, the file system was somehow corrupted. I later formatted it and copied the same files from a Windows box and everything was fine.
1. The file names need to be in the format of `ddddd.mp3` where `d` is a digit.