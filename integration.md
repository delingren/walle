# Integration

## Electronics

I am dividing the electronics into two parts. The lower part is mounted on the chassis near the drive train. It contains the MCU, the audio player, and the H-bridges. The upper part is basically a hub for everything related to the head, arms, IR remote, and the push button. The two parts are connected with a ribbon cable and Dupont connectors.

The power source is first fed to the switch. The switched power is then fed to both the upper and the lower parts. The switch is a three way one. In addition to on and off, there is also a "try me" mode. In that mode, two extra wires are connected. I didn't use it in v2. But here I am connecting one of them to `GND` and the other to a GPIO pin so that we can read the mode. So here's the finished switch, with one input and two outputs. The connectors are 2-pin JST RCYs. The female end can be directly plugged into standard 2.54mm Dupont male connectors. I had to enlarge the existing holes on the switch PCB using a small file. Otherwise it's rather trivial to cut existing wires and solder new ones. The following picture was taken before soldering wires for the mode switch.

![switch](./media/IMG_0359.jpeg)

I soldered all electronic components onto a perf board. It has the following interfaces:

* Power source
* Mode switch
* Left and right motors
* Speaker
* Connectors for the upper part

To help with soldering, I first drew all the connections using [Acorn](https://flyingmeat.com/acorn/), a simple image editing tool running on macos, capable of layer editing.

![pcb_diagram](./schematics/perfboard.png)

Here's the soldered perf board:

![lower_pcb](./media/IMG_1171.jpeg)

The PCB is then mounted to the chassis using a 3D printed mount.

![mounted_pcb](./media/IMG_0376.jpeg)

Now the upper part. I want to have a neat interface between the upper part and the lower part for easy assembly. So I use a small PCB as a hub and plug everything in the lower part into its pin headers. The PCB has a power bus connected to the switched power source. Then I use a flat ribbon cable to connect all the logic signals. The two IR receivers' signals are combined on the PCB and fed to the MCU via one pin. The MOSFET for the eye tilter motor is also integrated to the PCB. Here are the connections.

1. Eye tilt motor - GPIO 2 
2. IR receiver, front - GPIO 3
3. Eye, right - GPIO 4
4. Eye, left - GPIO 5
5. Head - GPIO 6
6. Arm, left - GPIO 7
7. Arm, right - GPIO 8
8. Push button - GPIO 9
9. IR receiver, back - GPIO 3

![upper_pcb](./media/IMG_0844.jpeg)

## Mechanics

For the main drive train, I am mounting the DC motors directly the to driving wheels, using 3d printed parts. The motors are connected to the outputs of the H-bridges. I was originally planning on using a couple of N20 5V 100rpm motors. But it turned out that they don't have enough torque. There is too much friction in the system (let's face it, none of the parts are precisely made and positioned, and there's no lubrication at all). So I ended up using two N30 6V 75rpm versions. They seemed to work fine.

![drive_train](./media/IMG_0816.jpeg)

For turning the head and tilting the eyes, I already described the mechanism [here](./head_rotation.md) and [here](./motor.md). The servos for driving the arms are described [here](./arm.md). Altogether, I am mounting the three servos and the DC motor on an upper platform.

![servos](./media/IMG_0897.jpeg)

## Calibration and debugging

Before working on the final sketch, I am going through the following steps to make sure all parts are mechanically functioning as expected.

For the upper part:

1. Verify eye LEDs
1. Verify IR remote receiver
1. Verify eye tilter motor
1. Verify push button
1. Verify arm and head servos
1. Calibrate servos to determine their limits

![upper](./media/IMG_0870.mov)

For the lower part:

1. Verify both DC motors can turn both directions and the speed can be controlled with PWM pins

![lower](./media/IMG_1213.mov)

Before closing everything up, I am also exposing a USB-C port for uploading sketches and debugging through the serial port. To do this, I cut a hole at the bottom of the chassis, and mounted a panel-mount USB-C port (bought from AliExpress). Then soldered a male USB-C plug on it. Note that I am not connecting the V+ wire, to avoid accidental current backflow to the computer. So it needs to be powered by the batteries when uploading the sketch.

![bottom_usb](./media/IMG_0888.jpeg)

Now all the mechanical parts are in place and properly assembled. It's time to play with the firmware.