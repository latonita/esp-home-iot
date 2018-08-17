#ifdef CO2_ON
#ifndef _AIR_QUALITY_H_
#define _AIR_QUALITY_H_

class AirQualitySensor {
private:
    //dht11 dht;
    unsigned char pin;
    double humidity = 50.0;
    double temperature = 22.0;

    const float alpha = 0.7;
    bool initialized = false;

public:
    AirQualitySensor(unsigned char pin);
    ~AirQualitySensor();

    const char * formattedTemperature();
    const char * formattedHumidity();
    void update(bool * readyForDHTUpdate);

    const char * getDataJson();
};

#endif
#endif
