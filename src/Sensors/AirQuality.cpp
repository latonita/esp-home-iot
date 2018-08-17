#include "../Config.h"
#ifdef CO2_ON
#include "AirQuality.hpp"
#include "../Sys/Utils.h"

AirQualitySensor::AirQualitySensor(unsigned char pin) : pin(pin) {}
AirQualitySensor::~AirQualitySensor() {}

void AirQualitySensor::update(bool * readyForDHTUpdate) {
    // *readyForDHTUpdate = false;
    // if (DHTLIB_OK == dht.read(pin)) {
    //     initialized = true;
    //     humidity = !initialized ? dht.humidity : alpha * humidity + (1 - alpha) * (double)dht.humidity;
    //     temperature = !initialized ? dht.temperature : alpha * temperature + (1 - alpha) * (double)dht.temperature;
    //     Serial.printf("DHT smoothed: Temp: %s C, Humi:%s%%\r\n", formattedTemperature(), formattedHumidity());
    // } else {
    //     Serial.println("DHT reading failure.");
    // }
}

const char * AirQualitySensor::formattedHumidity() {
    static char humi[8];
    strcpy(humi, formatDouble41(humidity));
    return humi;
}

const char * AirQualitySensor::formattedTemperature() {
    static char temp[8];
    strcpy(temp, formatDouble41(temperature));
    return temp;
}

const char * AirQualitySensor::getDataJson() {
    snprintf(utils_buff64, 64, "{\"temperature\":%.2f,\"humidity\":%.2f}", temperature, humidity);
    return utils_buff64;
}


#endif