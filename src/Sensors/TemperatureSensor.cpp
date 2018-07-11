#include "../Config.h"
#ifdef DHT_ON
#include "../Sys/Utils.h"
#include "TemperatureSensor.hpp"

TemperatureSensor::TemperatureSensor(unsigned char pin) : pin(pin) {}
TemperatureSensor::~TemperatureSensor() {}

void TemperatureSensor::update(bool * readyForDHTUpdate) {
    *readyForDHTUpdate = false;
    if (DHTLIB_OK == dht.read(pin)) {
        initialized = true;
        humidity = !initialized ? dht.humidity : alpha * humidity + (1 - alpha) * (double)dht.humidity;
        temperature = !initialized ? dht.temperature : alpha * temperature + (1 - alpha) * (double)dht.temperature;
        Serial.printf("DHT smoothed: Temp: %s C, Humi:%s%%\r\n", formattedTemperature(), formattedHumidity());
    } else {
        Serial.println("DHT reading failure.");
    }
}

const char * TemperatureSensor::formattedHumidity() {
    static char humi[8];
    strcpy(humi, formatDouble41(humidity));
    return humi;
}

const char * TemperatureSensor::formattedTemperature() {
    static char temp[8];
    strcpy(temp, formatDouble41(temperature));
    return temp;
}

const char * TemperatureSensor::getDataJson() {
    snprintf(utils_buff64, 64, "{\"temperature\":%.2f,\"humidity\":%.2f}", temperature, humidity);
    return utils_buff64;
}
#endif