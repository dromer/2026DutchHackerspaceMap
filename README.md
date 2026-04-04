This project was created for a soldering workshop held during International Open Hackespace Day 2026  (March 28th)  at techinc.nl, one of the Dutch hackerspaces. During preparations for this workshop it was discovered that another Dutch hacker space (revspace.nl) had previously also made an electronic map ("project decenium") for their 10th aniversery.

This Repository contains three directories:
- Hardware - with the KiCAD design files of the PCB
- Software - The software that runs on the map
- 3D - Design files for case(s) to hold the PCB
- Workshop - Instruction materials (crude) for the workshop


In it's basic form the map is a PCB with 18 2020 size ws2812 compatibe RGB LEDs on the front (representing the hackerspaces in NL), 6 capacitors and an ESP32C3 "supermini" module on the back. The ESP32 is retreiving information to drive the LEDs in specific colors: green when the space-state indicates the space is open, red when it is closed, blue when there is an (intermittant) communication issue, white when booting and a few other colours (not observed yet) indicating various rare conditions. The colours shown by the LEDs are animated

The PCB design lacks solder mask and copper outside the land boundaries of NL, showing the PCB base material, which is somewhat translucent. This can be used to mount a backlight behind the PCB. This backlight can either be a simple backlight or a 20 LED piece of common ws2812B LED strip. In the latter case it can be driven by the ESP32 as well.
