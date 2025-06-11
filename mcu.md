# Microcontroller

We need to choose a microcontroller. I am not going to make a custom PCB for this project. Instead, I will use a dev board and a perf board. There are a few factors to consider.

1. There needs to be enough pins.
1. Dimension. We want the board to be compact.
1. Power. Ideally, I want to be able to power the board with 4 AA batteries, so that I don't need an extra 5V DC regulator.

Here's a tally of the minimum pin needs. These pins will be explained in detail in later sections.

* PWM output: 7 pins
  - Left tread H bridge (1)
  - Right tread H bridge (1)
  - Left eye LED
  - Right eye LED
  - Eye tilt motor

* Digital output: 4 pins
  - Left tread H bridge (2)
  - Right tread H bridge (2)

* Servos: 3 pins. Generally, libraries use PWM to control servos. Some may use PIO on Pi Pico.
  - Left arm servo
  - Right arm servo
  - Head servo

* Input: 2 pins
  - IR sensor
  - Push button

* UART for audio: 2 pins. Note that not all pins can be used for UART.
  - Rx
  - Tx

I have a few [RP2040-Zero dev boards](https://www.waveshare.com/rp2040-zero.htm) in my drawer. They are compatible with Raspberry Pi Pico but has a much smaller footprint.

* Dimensions: 18 x 23.5
* Regulator: ME6217 (LDO, 800mA, max input 6.5V)
* GPIOs: 20 via the header, all capable of PWM.
* UART: 2; UART0 defaults to GPIO0(Tx) and GPIO1(Rx).

It seems to fit the bill really well. So I'll be using it during the prototyping phase and see if it works for everything.

First off, let's verify that all the GPIO pins are PWM capable by using a simple [sketch](./debug/pwm/) to dim LEDs. Sure enough, all these 20 pins are able to fade the LED. This takes care of the eyes. These pins should be good for controlling motors and servos as well. But let's verify on a breadboard in a minute.
