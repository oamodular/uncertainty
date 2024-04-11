# *Uncertainty is now OSHW! both the software and the hardware!*

This readme needs to be updated to reflect this and to talk about the many other languages that people have used to create new firmware for Uncertainty. Right now I just want to say that the 4hp DIY version of Uncertainty is still a work in progress. It's been built, but we're in the process of trying to figure out why it doesn't work. The 2hp SMT version of Uncertainty is the one that we sell [here](https://oamodular.org/products/uncertainty).

What remains below is the previous readme. It contains the pinout for Uncertainty's Seeeduino Xiao RP2040 and some example Python code…

# Hack Uncertainty for fun and profit

Uncertainty's firmware is open-source and easy to swap with a humble USB-C cable. You can write your own firmware in C++, MicroPython, CircuitPython, and probably a bunch of other languages. It's startlingly easy to work and this document will outline the whole deal. Let's go…

## Uncertainty’s Heart

We’re working with the mighty RP2040, a dual-core 133 MHz arm processor that is just a silly amount of power for outputting gates. Heck, you can easily overclock to over 300 MHz. Really hoping people make some messed up things.

## Uncertainty’s I/O

- one CV input that can recognize signals -5v to +5v (resolution is functionally close to 10-bit, so there probably isn’t quite the precision for v/oct)
- eight +5v gate outputs that each have their own LED (if you turn on the gate, you turn on the LED; they can’t be decoupled)
- a three pin I2C connector (this feature is experimental and largely untested)

To access this I/O use the following pins. You can use other pins, but they won’t work. I don’t know why you’d use other pins. Use these pins:


|        | Pin # |
| ------ | ------ |
|CV input|26|
|Gate 1|27|
|Gate 2|28|
|Gate 3|29|
|Gate 4|0|
|Gate 5|3|
|Gate 6|4|
|Gate 7|2|
|Gate 8|1|
|I2C SDA|6|
|I2C SCL|7|


## How to reinstall the default firmware

You hold down the boot button when you plug in the USB-C as described in the *Getting started* section. Then you drag ```uncertainty.uf2``` onto the RPI-RP2 disk. It should then unmount. All set! The same works for any .uf2 firmware.
