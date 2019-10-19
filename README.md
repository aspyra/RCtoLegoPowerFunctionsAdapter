# RC to Lego PF IR Adapter

This is a repository for an adapter between RC aperture signals and lego Power Functions IR Receivers. Watch the video first!

### Video

link

### Features

- Up to 4 IR channels
- Up to 5 RC channels (more possible with use of pin change interrupts)
- USB programming
- progammingless setup
- two LEDs for displaying the current state
- powered from lego 9V plug
- one RC channel can control many IR channels, one IR channel can be controlled by multiple RC channels
- 5x3x2 studs
- Combatibility mode for concurrect use of many PF remotes

### Arduino libraries used

The code uses the IRremote library: http://z3t0.github.io/Arduino-IRremote/.
It can be downloaded straight from Arduino IDE, search for 'IRremote' by shirriff.

### Parts

The project uses:
- A Lego PF cable
- AMS1117-5.0 voltage regulator
- BlueMX DSM2 RC receiver with deattached header
- Arduino Micro compatible Beetle board
- 3mm IR LED
- BC847 NPN SMD transistor
- standard pushbutton
- red SMD LED
- SMD resistors
- wires, cables
