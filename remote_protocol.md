# Remote Control Protocol

## Control Protocol

In v2, I sent one event in one frame. An event is either a button press or joystick position change. This was a little inefficient due to the overhead to payload ratio of the transmission, which is very slow. So I decided to improve it by sending the state of the whole remote control in each frame, in 48 bits. To send more than 32 bits, I am using the following function call.

```
  // Protocol: 2 Raw Code: 0x0000BA9876543210
  uint8_t buffer[6] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA};
  IrSender.sendPulseDistanceWidthFromArray(
    &NECProtocolConstants, 
    (IRRawDataType*)buffer, 
    8 * sizeof(buffer)/sizeof(buffer[0]), 
    0);
```

which is recognized as protocol 2 on the receiver side.

I noticed that the hobby level joysticks are not very accurate. In v1 and v2, I used 16 bits (8 each axis) for each pair of joystick coordinates. It turned out to be a big overkill. A resolution of 4 bits per axis is good enough. So I decided to use just one byte for each coordinate. Each coordinate takes a value of `[-7,7]` where `0` is the neutral position. To make things simple, I use 4 bits signed magnitude encoding to represent each coordinate. There are two reasons I use this subpar encoding, which has two representations of `0`:

1. I need an odd number of codes for symmetry. A two's complement encoding gives me `[-8,7]`. I waste a code anyway.
1. For some reason, the IR transmission doesn't work well with all zero bits. So I am using `b1000` to represent zero to avoid all zeros. I call that a happy incident.

## Code Format and Strategy
The following is the code format I use.
* Bits 47-40: xor of other 5 bytes, for error detection purpose. 
* Bits 39-16: bitmap representing button states. 0 for released and 1 for pressed.
* Bits 15-0: coordinates of two joystick.

Since the error rate is rather high, especially at the edge of the range limit, I decide to use an error detection byte so that the receiver can discard bad transmissions. Also due to lack of reliability of IR transmission, I need to repeat packets even if the state has not changed on the gamepad. But I also don't want to repeatedly transmit packets indefinitely when the gamepad is just idling. So I implemented an exponential back off strategy. When a repeat packet is sent, the interval before the next one is sent is increased by 1.5x. This way, when the gamepad stops changing, we'll send multiple repeats in short succession. But it'll slow down exponentially quickly.

## Translate Joystick Positions into Motor Control
Wall-E's movements are controlled by two treads, just like a tank or a bulldozer. The left and right treads can move independently. And I want to use a joystick to control its movements. Since a 2-axis joystick has 2 degrees of freedom, just like the treads, it should be possible to map the joystick positions to the tread movements precisely. And I want the mapping to be intuitive as well. It turned out that this isn't super trivial. So I am going to elaborate a little in this section.

Let's denotes the movements of the two treads first. They can move forward or backwards. Let's use 1 to denote a full speed forward move, -1 for a full speed backward move, and 0 for no movements. So, for Wall-E to move forward full speed, we need a (1,1); a full speed backward movement can be achieved by a (-1,-1). To spin in place left, we need a (-1,-1); to make a left turn pivoting on the left tread, we need a (0,1), etc etc.

Now consider the movements of the joystick. When the joystick is in the neutral position, there should be no movements. We want the control to be intuitive. So, pushing the joystick forward and backward along the Y-axis should make Wall-E to move along a straight line and the direction and speed are dependent on the joystick's position relative to the neutral position. Similarly, when the joystick moves along the X-axis, we should be spinning in place. When the joystick is in the upper-left corner, we should be making a left turn pivoting on the left tread. And so on and so forth.

Now, we can plot the nine key positions and the tread values. And we can draw arrows to represent the joystick movements from one position to another and the trends of the tread values along the way. And we will represent the position of the joystick with a normalized 2-dimensional coordinate. `(-1,1)*(-1,1)`.

```
            l-               l=
 (0,1)    ←────    (1,1)    ────→    (1,0)
            r=               r-
    ↑                ↑                 ↑
  l+│r=            l+│r+             l=│r+
    │                │                 │    
            l-               l+
(-1,1)    ←────    (0,0)    ────→    (1,-1)
            r+               r-
    │                │                 │
  l=│r-            l-│r-             l-│r=    
    ↓                ↓                 ↓
            l=               l+
(-1,0)    ←────   (-1,-1)   ────→    (0,-1)
            r+               r=
```

Then we can interpolate all the positions in between. Ideally, the range of motion of the joystick would be a circle. But a square will also do. The logic is going to be slightly more complicated. The following formula satisfies all the key positions and tread lines.

```
if (sign(x) == sign(y)) {
  r = sign(y)*max(|x|,|y|);
  l = sign(y)*(|y|-|x|);
} else {
  l = sign(y)*max(|x|,|y|);
  r = sign(y)*(|y|-|x|);
}
```

## Control Arms with a Joystick
I want to use the second joystick to control the arms' movements. Since the arms have two degrees of freedom between them, and the joystick has two axes, we should be able to do that. The problem is to find an intuitive way. Here are my goals:

* Forward/backward: raise or lower both arms
* Front left/back left: raise or lower left arm only
* Front right/back right: raise or lower right arm only

The remote control sends the raw positions of the joystick, just like the one that controls the treads. On the receiver (Wall-E) end, here's the logic.

* Determine if the joystick is to the left or right or in the middle, using the offset of the X axis.
* Raise or lower one or both arms depending on the previous result.
* The amount to raise or lower is determined by the offset of the Y axis.
