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