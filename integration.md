# Integration


## Electronics

I am dividing the electronics into two parts. The lower part is mounted on the chasis near the drive train. It contains the MCU, the audio player, and the H-bridges. The upper part is basically a hub for all everything related to the head, arms, IR remote, and the push button. The two parts are connected with a ribbon cable and Dupont connectors.

The power source is first fed to the switch. The switched power is fed to both the upper and the lower parts. The switch is a three way one. In addition to on and off, there is also a "play" mode. In that mode, the brown and purple wires are connected. I could potentially utilize it and read it in the MCU. I am not really sure what to do with the information though. So I am skipping it and keeping it simple. So here's the finished switch, with one input and two outputs. The connectors are 2-pin JST RCYs. The female end can be directly plugged into standard 2.54mm Dupont male connectors. I had to enlarge the exisitng holes on the switch PCB using a small file. Otherwise it's rather trivial to cut existing wires and solder new ones.

![switch](./media/IMG_0359.jpeg)

I managed to solder everything in the lower part onto a 50mmx70mm perf board. It has the following interfaces:

* Power source
* Left and right motors
* Speaker
* Connectors for the upper part

![lower_pcb](./media/IMG_0357.jpeg)

The PCB is then mounted to the chasis using a 3D printed mount.

![mounted_pcb](./media/IMG_0828.jpeg)

Now the upper part. I want to have a neat interface between the upper part and the lower part for easy assembly. So I use a small PCB as a hub and plug everything in the upper part into its pin headers. The PCB has a power bus connected to the switched power source. Then I use a flat ribbon cable to connect all the logic signals. The two IR receivers' signals are combined on the PCB and fed to the MCU via one pin. The MOSFET for the eye tilter motor is also integrated to the PCB. Here are the connections.

1. Eye tilt motor - GPIO 2 
2. IR receiver, front - GPIO 3
3. Eye, left - GPIO 4
4. Eye, right - GPIO 5
5. Arm, right - GPIO 6
6. Head - GPIO 7
7. Push button - GPIO 8
8. Arm, left - GPIO 9
9. IR receiver, back - GPIO 3

![upper_pcb](./media/IMG_0844.jpeg)