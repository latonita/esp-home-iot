//
//  dht_util.cpp
//  esp8266_WeatherStationDemo
//
//  Created by <author> on 04/05/2017.
//
//

#include "dht_util.hpp"
#include "utils.h"
#include <ArduinoJson.h>

dht_util::dht_util(char pin) : pin(pin) {}
dht_util::~dht_util() {}

void dht_util::update(bool * readyForDHTUpdate) {
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

const char * dht_util::formattedHumidity() {
    static char humi[8];
    strcpy(humi, formatDouble41(humidity));
    return humi;
}

const char * dht_util::formattedTemperature() {
    static char temp[8];
    strcpy(temp, formatDouble41(temperature));
    return temp;
}

const char * dht_util::getDataJson() {
    DynamicJsonBuffer json;
    JsonObject & root = json.createObject();
    root["temperature"] = String(temperature);
    root["humidity"] = String(humidity);

    static char dataBuffer[64];
    root.printTo(dataBuffer);
    return dataBuffer;
}
