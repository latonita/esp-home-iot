//
//  dht_util.cpp
//  esp8266_WeatherStationDemo
//
//  Created by <author> on 04/05/2017.
//
//

#include "dht_util.hpp"
#include "utils.h"

dht_util::dht_util(char pin) : pin(pin) {}
dht_util::~dht_util() {}

void dht_util::update(bool * readyForDHTUpdate) {
    *readyForDHTUpdate = false;
    if (DHTLIB_OK == dht.read(pin)) {
        initialized = true;
        humidity = !initialized ? dht.humidity : alpha * humidity + (1 - alpha) * (double)dht.humidity;
        temperature = !initialized ? dht.temperature : alpha * temperature + (1 - alpha) * (double)dht.temperature;
        Serial.printf("DHT smoothed: Temp: %s°C, Humi:%s%%\r\n", formattedTemperature(), formattedHumidity());
    } else {
        Serial.println("DHT reading failure.");
    }
}

char * dht_util::formattedHumidity() {
    return formatDouble41(humidity);
}

char * dht_util::formattedTemperature() {
    return formatDouble41(temperature);
}
