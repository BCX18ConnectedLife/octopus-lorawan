# Octopus LoRa Radio Example
An example Arduino sketch to demonstrate how to use the [Octopus board](http://fab-lab.eu/octopus/) in 
combination with an [Adafruit LoRa Radio FeatherWing](https://learn.adafruit.com/radio-featherwing) to send sensor telemetry
using Low-Power Wide-Area networks using LoRaWAN.

The example uses the sensors Bosch BME680 (Environment) and Bosch BNO0555 (Accelerometer)
to periodically send sensor data using the [Cayenne Low Power Payload (CayenneLPP)](https://github.com/myDevicesIoT/cayenne-docs/blob/master/docs/LORA.md) format.

The LoRaWAN communication part (Joining the network with OTAA and usage of the LMIC framework)
is based on work of Thomas Telkamp and Matthijs Kooijman which
can be found here: https://github.com/matthijskooijman/arduino-lmic/blob/master/examples/ttn-otaa/ttn-otaa.ino

## Arduino IDE Setup
Follow the instructions given here:
https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide

When finished, you should be able to get the "Blink Test" sketch running successfully.

## LMIC Library for Arduino
Install the LMIC library for Arduino.

Sketch --> Include Library --> Manage Libraries ...
Search for "LMIC"

Install "IBM LMIC Framework - Arduino Port of the LMIC"

## Sensor Libraries
Install library "Adafruit Unified Sensor"

Install library "Adafruit BME680 Library"

Install library "Adafruit BNO055"

# Other libraries
Install library "CayenneLPP" by The Things Network

