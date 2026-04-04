# 2026 Dutch Hackerspace map

## General

This project was created for a soldering workshop held during International Open Hackespace Day 2026  (March 28th)  at techinc.nl, one of the Dutch hackerspaces. During preparations for this workshop it was discovered that another Dutch hacker space (revspace.nl) had previously also made an electronic map ("project decenium") for their 10th aniversery.

This Repository contains three directories:
- Hardware - with the KiCAD design files of the PCB
- Software - The software that runs on the map
- 3D - Design files for case(s) to hold the PCB
- Workshop - Instruction materials (crude) for the workshop

In it's basic form the map is a PCB with 18 2020 size ws2812 compatibe RGB LEDs on the front (representing the hackerspaces in NL), 6 capacitors and an ESP32C3 "supermini" module on the back. The ESP32 is retreiving information to drive the LEDs in specific colors: green when the space-state indicates the space is open, red when it is closed, blue when there is an (intermittant) communication issue, white when booting and a few other colours (not observed yet) indicating various rare conditions. The colours shown by the LEDs are animated

The PCB design lacks solder mask and copper outside the land boundaries of NL, showing the PCB base material, which is somewhat translucent. This can be used to mount a backlight behind the PCB. This backlight can either be a simple backlight or a 20 LED piece of common ws2812B LED strip. In the latter case it can be driven by the ESP32 as well.

## Hardware

The "hardware" folder contains the schematic diagram and KiCAD source files for this project. You should be able to produce gerber files, which can be processed into actual PCBs by any board house

IMPORTANT NOTICE
Please leave pin d9 of the ESP32C3 module disconnected. This is the boot mode selection pin, and connecting it to DO of the last WS281b will result in the ESP32 reliably booting into software upload mode - rendering the system non-functional. You can remove or cut the D9 pin from the module before soldering it onto the PCB or you can cut the trace leading to it from the last LED. If you want to use WS2812b based led strip as a backlight, then the DI pin of the led strip should be connected to the DO pin of the last LED - so either the PAD underneath the D9 pin of the ESP32 or the trace leading to it. THIS PROBLEM MAY BE FIXED IN NEW RELEASES OF THE HARDWARE

## Software

The "software" folder contains a self-contained Arduino application.

## Installing and compiling

As Espressif regulary makes breaking changes to their API, it is important that you use the __correct version of the Espressif framework for Arduino: 3.1.3__

After you have installed the Espressif board manager package, you can __select the "ESP32-C3 Dev-module"__, which is compatible with the ESP32-C3 "super-mini" module that is usedin this project.

Please make sure that you have __set the option "USB CDC enable on boot" to "enabled"__ (Arduino tools menu), and that you have selected the correct serial device. You may have to press and hold the boot-button while presssing and releasing the reset button to put the module into "upload mode".

.
### Notes on driving the LEDs with the "RMT" (remote control) perhipheral

The  "RMT" perhipheral.Can generate very accurately timed signals, and is Espressif's way of driving WS2812 LEDs. Unfortunately it has enormous memory overhead: it requires 32 times more storage than there is raw data to be sent to the LEDs. 24 bits to be sent to each LED requires 768 bits of memory. Every RGB byte translates to 8 32 bit word accesses. This method uses DMA (Direct memory access) to relieve the processor of these duties: The processor just needs to prepare a block of data and then start the transfer.

### Notes on driving the display with the "SPI" (synchronous serial port) perhipheral 

This program uses the "SPI" perhipheral. Signals are less accuratly timed, but do actually work with the types of LEDs tested so far. Overhead is much lower than for the "RMT" method: it requires 4 times more storage than there is raw data to be sent to the LEDs. 24 bits to be sent to each LED requires 96 bits of memory. Moreover, as every (RGB) byte access becomes a single 32 bit word access, processor overhead is relatively limited. This method uses DMA (Direct memory access) to relieve the processor of these duties: The processor just needs to prepare a block of data and then start the transfer.


