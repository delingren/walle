# Custom Remote Control

In addition to using an existing remote control, I also wanted to make my own remote control. It would have at least a 2-axle joystick and a few buttons. The biggest challenge is probably 3d pringing the housing. So I am exploring the option of using an existing PS/2 control, gutting the insides and replacing it with a microcontroller.

To do that, there are a few things I need to figure out.

1. The joysticks. A PS/2 controller has two of them. They are both 2-axle joysticks. I assume they are resistive (i.e. pototiometers) and I should be able to use an ADC pin to read the position of an axle.
1. The IR emitter. This should be relatively easy. The library I use on the receiver side is also capable of sending.
1. The microcontroller. Since the PS/2 controller is powered by 2 AAA batteries, I want to be able to reuse that. So I need an mcu that can be powered at 3V. First thing that came to my mind is ATmega 328. I have a couple of them and they are 8MHz versions.

## Microcontroller
As mentioned before, I intend to use an 8MHz ATmega328 devboard (Pro Mini clones), which should be able to run at 3V comofortably. So I am going to prototype everything on that board.


The MCU itself can be powered by 1.8-5.5V. And since I'm using batteries, I don't need the regulator. Removing it will increase the battery life and eliminate the voltage drop. 

## IR transmitter
I have some generic 940 nm IR LEDs from Amazon and I intend to use one. Since I only need short distance transmissions, I should be able to use an MCU pin to control it directly, without a BJT.

## Joystick Hardware

## Joystick Movements

Wall-E's movements are controlled by two treads, just like a tank or a bulldozer. The left and right treads can move independently. And I want to use a joystick to control its movements. Since a 2-axle joystick has 2 degrees of freedom, just like the treads, it should be possible to map the joystick positions to the tread movements precisely. And I want the mapping to be intuitive as well. It turned out that this isn't super trivial. So I am going to elaborate a little in this section.

Let's denotes the movements of the two treads first. They can move forward or backwards. Let's use 1 to denote a full speed forward move, -1 for a full speed backward move, and 0 for no movements. So, for Wall-E to move forward full speed, we need a (1,1); a full speed backward movement can be achieved by a (-1,-1). To spin in place left, we need a (-1,-1); to make a left turn pivoting on the left tread, we need a (0,1), etc etc.

Now consider the movements of the joystick. When the joystick is in the neutral position, there should be no movements. We want the control to be intuitive. So, pushing the joystck forward and backward along the Y-axle should make Wall-E to move along a straight line and the direction and speed are dependent on the joystick's position relative to the neaural position. Similarly, when the joystick moves along the X-axle, we should be spinning in place. When the joystick is in the upper-left corner, we should be making a left turn pivoting on the left tread. And so on and so forth.

Now, we can plot the nine key positions and the tread values. And we can draw arrows to represent the joystick movements from one position to another and the trends of the tread values along the way. And we will represent the position of the joystick with a normalized 2-dimensional coordiate. `(-1,1)*(-1,1)`.

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
  l = sign(y)*max(|x|,|y|);
  r = sign(y)*(|y|-|x|);
} else {
  r = sign(y)*max(|x|,|y|);
  l = sign(y)*(|y|-|x|);
}
```

## Control Protocol

Each transmission contains 32 bits, which should be more than enough for a low speed application. Here's the format of the packets.

* Bits 0 - 7: type of packet
  - 1: Button state
  - 2: Joystick 1
  - 3: Joystick 2

* Bits 8 - 15: reserved
* Bits 16- 31: dependent on the type of packet
  - 1: bit
  - 2 & 3: bits 16 - 31 contain the X and Y coordinates. Each coordinate is a signed 8 bit integer. This gives us the range of [-127, 128] for either axle.
