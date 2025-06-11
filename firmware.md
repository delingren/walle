# Firmware

There are two independent parts. One for the main toy and one for the remote control. I'm using Arduino framework for both.

## Main

The firmware is relatively straight-forward. One thing worth mentioning is animation. When we receive a command, such as turning the head, we want the operation to be rather smooth. So, instead of setting the position immediately, we animate the transition to the target position. Animation is also needed to create breathing patterns in the eyes. This also applies to the speed of the motors so that they accelerate and decelerate smoothly. So I am defining an `Animatable` class and keep a list of ongoing animations at any moment.

There are at least these two orthogonal aspects to an animation.
1. Absolute value / relative value
1. Transition time / transition speed

So I have defined all these permutations, in addition to a constant animation (or in-animation?). All the transitions are linear. This should be enough for this simple application. Every time we execute `loop()`, i.e. in each frame, we update all the status of all animations.

1. For an ongoing animation, we calculate its transient value by interpolating using the start time, duration, initial value, and target value.
1. If an animation has reached its target value, we remove it from the list and enqueue the next animation.
1. When a new animation is enqueued, we calculate its target value if its value is specified in relative terms.

### Libraries

Instead of relying on Arduino IDE's library manager, I enlisted DYPlayer and IRRemote libraries as submodules and froze them at the current version. So that it's more self contained and resistant to future breaking changes to these libraries. The Servo library, however, is part of RP2040 board SDK which uses PIO instead of PWM. There is no need to enlist it as a submodule, unless I also enlist the whole SDK, which is unnecessarily complicated.

After enlisting these two libraries, I linked all files in their `src` directories under my sketch's `src` directory. Arduino Framework link all source files under `src` directory. I don't know if there is a way to add a local library. But creating symbolic links is pretty straight-forward with the following bash commands.

```
for i in ../../third_party/Arduino-IRremote/src/* ; do; ln -s $i ; done
for i in ../../third_party/dyplayer/src/* ; do; ln -s $i ; done
```

## Remote Control

The remote control protocol is described in [its own chapter](./remote_protocol.md). The code implementing the sender side simply scans the state of all the controls and sends out a packet. Since the transmission rate is rather low (10 frame per second at most), we don't need to do any debouncing for the buttons. The only interesting bit is scanning the matrix. I'm scanning from column to row. All row pins are pulled up high by default. When we scan, we pull down the column row. If a key at the intersection of the col and row is pressed, it pulls down its row pin. So we can get the state of each button by scanning all the columns and reading all the rows.