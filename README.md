# Octopus LoRa Radio Example
An example Arduino sketch to demonstrate how to use the [Octopus board](http://fab-lab.eu/octopus/) in 
combination with an [Adafruit LoRa Radio FeatherWing](https://learn.adafruit.com/radio-featherwing) to send sensor telemetry
using Low-Power Wide-Area networks using LoRaWAN.

The example uses the sensors [Bosch BME680](https://www.bosch-sensortec.com/bst/products/all_products/bme680) (Environment) 
and [Bosch BNO0555](https://www.bosch-sensortec.com/bst/products/all_products/bno055) (Accelerometer)
to periodically send sensor data using the [Cayenne Low Power Payload (CayenneLPP)](https://github.com/myDevicesIoT/cayenne-docs/blob/master/docs/LORA.md) format.

The LoRaWAN communication part (Joining the LoRa network with OTAA and using the LMIC framework)
is based on work of Thomas Telkamp and Matthijs Kooijman which
can be found here: https://github.com/matthijskooijman/arduino-lmic/blob/master/examples/ttn-otaa/ttn-otaa.ino

Instructions for assembly and wiring it up can be found here: [ASSEMBLY.md](ASSEMBLY.md).

![Octopus and LoRa FeatherWing](media/octopus_featherwing.png)

## Arduino IDE Setup
Follow the instructions given here:
https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide

When finished, you should be able to get the "Blink Test" sketch running successfully.

### Libraries
You need to install several libraries in the Arduino IDE

Sketch --> Include Library --> Manage Libraries ...

* "IBM LMIC Framework - Arduino Port of the LMIC"
* "Adafruit Unified Sensor"
* "Adafruit BME680 Library"
* "Adafruit BNO055"
* "CayenneLPP" by The Things Network

