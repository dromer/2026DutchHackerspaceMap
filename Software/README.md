This folder contains a self-contained Arduino application.

### Installing and compiling

As Espressif regulary makes breaking changes to their API, it is important that you use the __correct version of the Espressif framework for Arduino: 3.1.3__

After you have installed the Espressif board manager package, you can __select the "ESP32-C3 Dev-module"__, which is compatible with the ESP32-C3 "super-mini" module that is usedin this project.

Please make sure that you have __set the option "USB CDC enable on boot" to "enabled"__ (Arduino tools menu), and that you have selected the correct serial device. You may have to press and hold the boot-button while presssing and releasing the reset button to put the module into "upload mode".

#### PlatformIO

You can also build and flash by executing `pio run -t upload` from this folder.


### Notes on driving the LEDs with the "RMT" (remote control) perhipheral

The  "RMT" perhipheral.Can generate very accurately timed signals, and is Espressif's way of driving WS2812 LEDs. Unfortunately it has enormous memory overhead: it requires 32 times more storage than there is raw data to be sent to the LEDs. 24 bits to be sent to each LED requires 768 bits of memory. Every RGB byte translates to 8 32 bit word accesses. This method uses DMA (Direct memory access) to relieve the processor of these duties: The processor just needs to prepare a block of data and then start the transfer.

### Notes on driving the display with the "SPI" (synchronous serial port) perhipheral 

This program uses the "SPI" perhipheral. Signals are less accuratly timed, but do actually work with the types of LEDs tested so far. Overhead is much lower than for the "RMT" method: it requires 4 times more storage than there is raw data to be sent to the LEDs. 24 bits to be sent to each LED requires 96 bits of memory. Moreover, as every (RGB) byte access becomes a single 32 bit word access, processor overhead is relatively limited. This method uses DMA (Direct memory access) to relieve the processor of these duties: The processor just needs to prepare a block of data and then start the transfer.
