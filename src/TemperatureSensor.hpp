#ifndef TemperatureSensor_hpp
#define TemperatureSensor_hpp
#include "dht11.h"

class TemperatureSensor {
private:
    dht11 dht;
    unsigned char pin;
    double humidity = 50.0;
    double temperature = 22.0;

    const float alpha = 0.7;
    bool initialized = false;

public:
    TemperatureSensor(unsigned char pin);
    ~TemperatureSensor();

    const char * formattedTemperature();
    const char * formattedHumidity();
    void update(bool * readyForDHTUpdate);

    const char * getDataJson();
};

#endif
