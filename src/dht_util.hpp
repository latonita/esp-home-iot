//
//  dht_util.hpp
//  esp8266_WeatherStationDemo
//
//  Created by <author> on 04/05/2017.
//
//

#ifndef dht_util_hpp
#define dht_util_hpp
#include "dht11.h"

class dht_util {
private:
    dht11 dht;
    char pin;
    double humidity = 50.0;
    double temperature = 22.0;

    const float alpha = 0.7;
    bool initialized = false;

public:
    dht_util(char pin);
    ~dht_util();

    char * formattedTemperature();
    char * formattedHumidity();
    void update(bool * readyForDHTUpdate);
protected:

};

#endif /* dht_util_hpp */
