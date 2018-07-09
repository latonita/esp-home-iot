#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "ConfigBase.h"

#define SERIAL_BAUD_RATE 115200 //however esp8266 boots at 74880 baud after power on.
#define UTC_OFFSET 3

#ifdef MAKE_POWER_METER
    // Pins
    #define LED_PIN 15
    #define LED_PIN2 2
    #define LED1 LED_PIN
    #define LED2 LED_PIN2

    #define POWER_PULSE_PIN 14
    #define DOORBELL_PIN 13 
    #define DHT_PIN 12

    #define SDA_PIN 5
    #define SDC_PIN 4

    #define I2C_DISPLAY_ADDRESS 0x3c
    #define POWER_PULSES_PER_WATT_HOUR 1.60 //1600 pulses = 1000 W*h
    #define DHT_UPDATE_INTERVAL_SECS 1 * 60 // read from DHT sensor

    // Features
    #define DISPLAY_ON
    #define DHT_ON
    #define WEATHER_ON
    #define POWER_ON
    #define DOORBELL_ON
#endif // MAKE_POWER_METER

#ifdef MAKE_WATER_METER
    #define WATER_HOT_PULSE_PIN 13    // kwh pulses
    #define WATER_COLD_PULSE_PIN 16    // kwh pulses

    #define WATER_COLD_PULSES_PER_LITER 1
    #define WATER_HOT_PULSES_PER_LITER 1

    #define WATER_ON
#endif


#endif