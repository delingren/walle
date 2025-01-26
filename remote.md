# IR Remote Control

The original toy came with a remote control but none of mine had it when I bought them. Not that I really want to use it though. Instead, I want to be able to use an existing TV remote control. In addition, I want to make my own. Wall-E has two IR sensors, one on the shoulder and one on the back. I can either reuse them if they work or replace them with my own if needed.

First order of business is to verify that the sensors respond my remote control. I have collected some remote controls from e-waste bins in the past. It's time to put them to good use. You probably have figured out by now that dumpster diving is one of my hobbies.

Different manufacturers have different IR remote protocols. Many are either publicly available or has been reverse engineered. To decode the signals, I am going to use this [Arduino-IRremote library](https://github.com/Arduino-IRremote/Arduino-IRremote).

I also want to be able to make my own joystick IR remote. To do that, I can either devise my own protocol, since I have full control on both sides, or piggyback on an existing one, as long as it's capable of encoding all the info I need. The IR library I use is capable of both decoding and encoding.

Here's a very comprehensive tutorial on this topic:
https://dronebotworkshop.com/ir-remotes/#SimpleSender_Example_Code
