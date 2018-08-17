#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "ConfigBase.h"
#include "ver.h"

#define SERIAL_BAUD_RATE 115200 //however esp8266 boots at 74880 baud after power on.
#define UTC_OFFSET 3

#ifdef MAKE_POWER_METER
    #define LED_POWER_PULSE 15
    #define LED_INFO 2

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
    #define WATER_COLD_PULSE_PIN 13
    #define WATER_HOT_PULSE_PIN  12

    #define LED_INFO 5

    #define WATER_COLD_PULSES_PER_LITER 1
    #define WATER_HOT_PULSES_PER_LITER 1

    #define WATER_ON
#endif

#ifdef MAKE_CO2_METER
    #define MQ135_PIN  12

    #define LED_INFO 16

    #define CO2_ON
#endif


#endif