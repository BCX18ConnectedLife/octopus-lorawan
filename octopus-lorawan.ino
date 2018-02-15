/*
 * Bosch SI Example Code License Version 1.0, January 2016
 *
 * Copyright 2016 Bosch Software Innovations GmbH ("Bosch SI"). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * BOSCH SI PROVIDES THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE
 * QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL
 * NECESSARY SERVICING, REPAIR OR CORRECTION. THIS SHALL NOT APPLY TO MATERIAL DEFECTS AND DEFECTS OF TITLE WHICH BOSCH
 * SI HAS FRAUDULENTLY CONCEALED. APART FROM THE CASES STIPULATED ABOVE, BOSCH SI SHALL BE LIABLE WITHOUT LIMITATION FOR
 * INTENT OR GROSS NEGLIGENCE, FOR INJURIES TO LIFE, BODY OR HEALTH AND ACCORDING TO THE PROVISIONS OF THE GERMAN
 * PRODUCT LIABILITY ACT (PRODUKTHAFTUNGSGESETZ). THE SCOPE OF A GUARANTEE GRANTED BY BOSCH SI SHALL REMAIN UNAFFECTED
 * BY LIMITATIONS OF LIABILITY. IN ALL OTHER CASES, LIABILITY OF BOSCH SI IS EXCLUDED. THESE LIMITATIONS OF LIABILITY
 * ALSO APPLY IN REGARD TO THE FAULT OF VICARIOUS AGENTS OF BOSCH SI AND THE PERSONAL LIABILITY OF BOSCH SI'S EMPLOYEES,
 * REPRESENTATIVES AND ORGANS.
 */

/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Adafruit_BNO055.h>
#include <CayenneLPP.h>

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]= { 0x69, 0x45, 0x00, 0xF0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]= { 0x00, 0x00, 0x3B, 0x90, 0x10, 0xB6, 0x76, 0x98 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = { 0x1C, 0x4C, 0x42, 0x7B, 0xF0, 0x95, 0x09, 0xA2, 0x39, 0x51, 0x37, 0x91, 0xE3, 0x32, 0xAF, 0x61 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

Adafruit_BME680 bme; // I2C
Adafruit_BNO055 bno055 = Adafruit_BNO055(55);

const float temperature_offset = 8.0;

static CayenneLPP payload(32);
static osjob_t sendjob;
static osjob_t blink_fast_job;

static boolean ledIsOn = false;
static boolean joined = false; // LoRa "Join" operation was successful

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 120;

// Pin mapping
// Adapted for Octopus board and Adafruit LoRa Radio FeatherWing.
// Wiring proposed by Guido Burger:
// RST -> Not connected
// DIO0 (aka IRQ) -> Connected to "B" using a Diode
// DIO1 -> Connected to "B" using a Diode
const lmic_pinmap lmic_pins = {
    .nss = 2, // "E"
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {15, 15, LMIC_UNUSED_PIN}, // "B", DIO0 (aka IRQ) and DIO1 are "ORed" using Diodes
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            joined = false;
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            joined = true;

            // Disable link check validation (including ABR)
            LMIC_setLinkCheckMode(0);
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        bme.performReading();

        payload.reset();
        payload.addTemperature(0x01, bme.temperature - temperature_offset);
        payload.addBarometricPressure(0x01, bme.pressure / 100.0);
        payload.addRelativeHumidity(0x01, bme.humidity);
        payload.addAnalogInput(0x01, bme.gas_resistance / 1000.0);

        imu::Vector<3> bnoAccel = bno055.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
        Serial.print("Acceleration (m/s^2) x=");
        Serial.print(bnoAccel.x());
        Serial.print(", y=");
        Serial.print(bnoAccel.y());
        Serial.print(", z=");
        Serial.println(bnoAccel.z());

        payload.addAccelerometer(0x01, bnoAccel.x(), bnoAccel.y(), bnoAccel.z());
        
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, payload.getBuffer(), payload.getSize(), 0);
        Serial.println(F("Packet queued"));

        Serial.print("Temperature=");
        Serial.print(bme.temperature - temperature_offset);
        Serial.print(" °C / ");
        Serial.print("Pressure=");
        Serial.print(bme.pressure / 100.0);
        Serial.print(" hPa  / ");
        Serial.print("Humidity = ");
        Serial.print(bme.humidity);
        Serial.print(" % / ");
        Serial.print("Gas = ");
        Serial.print(bme.gas_resistance / 1000.0);
        Serial.println(" KOhms");
    }
    
    // Next TX is scheduled after TX_COMPLETE event.
}

/*
 * Blink LED fast as long as not "joined" with the network
 */
void do_blink_fast(osjob_t* j) {
  if (!joined) {
    if (ledIsOn) {
      digitalWrite(0, LOW);
    } else {
      digitalWrite(0, HIGH);
    }

    ledIsOn = !ledIsOn;
  } else {
    digitalWrite(0, HIGH); // Turn off if joined
  }

  os_setTimedCallback(&blink_fast_job, os_getTime() + ms2osticks(500), do_blink_fast);
}

void setup() {
    pinMode(0, OUTPUT); // LED
    
    Serial.begin(115200);

    Serial.print("Initializing BME680: ");
    if (!bme.begin(0x76)) { // BME680 listens on I2C address 0x76
      Serial.println("Could not find a valid BME680 sensor, check wiring!");
      while (1);
    }

    // Set up oversampling and filter initialization
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms
    Serial.println("OK");

    Serial.print("Initializing BNO055: ");
    if (bno055.begin()) {
        bno055.setExtCrystalUse(true);
        Serial.println("OK");
    } else {
        Serial.println("Not found");
    }
    
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);

    do_blink_fast(&blink_fast_job);
}

void loop() {
    os_runloop_once();
}

