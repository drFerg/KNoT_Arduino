KNoT - Knowledgeable Network of Things
========================================


A 4th year undergraduate project to design and implement a lightweight communication protocol for the internet of things.

This is an Arduino based implementation of the KNoT protocol (based on the Panstamp hardware).

The protocol considers three types of general devices:
- Sensor 
- Actuator 
- Controller


Compiling
---------

In order to compile the code the Arduino IDE needs to be installed, as well as the TimerOne library for Arduino.
Next a symlink needs to be created from the libs/KNoT directory to the arduino/libraries directory in your home folder(Ubuntu).

After that, open and navigate the Arduino IDE into one of the devices directories (controller/sensor) and hit compile (Ctrl+r).

