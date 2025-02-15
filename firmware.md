# Firmware

I'm using Arduino framework for both the Wall-E and remote control sides.

## Wall-E

The firmware is relatively straight-forward. One thing worth mentioning is animation. When we recieve a command, such as turning the head, we want the operation to be rather smooth. So, instead of setting the position immediately, we animate the transition to the target position. This also applies to the speed of the motors as well as the brightness of the LEDs. So I am defining an `Animatable` class and keep a list of ongoing animations at any moment.

Then I have three kinds of animations.
1. Linear transition to a target state over a duration.
1. Instantly transition to a target state.
1. Maintain a constant value for a duration.

We can of course define other types of animations. But this list should be enough for this simple application. Every time we execute `loop()`, i.e. in each frame, we update all the animation status.

1. For an ongoing animation, we calculate its transient value by interpolating using the start time, duration, initial value, and target value.
1. If an animation has reached its target value, we remove it from the list.
1. We also read input from all the sources (remote control and pushbutton) and queue animations as needed.

### Libraries
* DYPlayer
* IRRemote
* Servo. This is part of RP2040 board SDK and it uses PIO instead of PWM. There is no need to enlist it as a submodule.

## Remote Control