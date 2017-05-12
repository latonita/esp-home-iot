#ifndef _LATONITA_UTILS_H_
#define _LATONITA_UTILS_H_

#include <stdlib.h>
#include <stdint.h>
#include <Arduino.h>

//system setting
#define MAX_UPTIME 4294967295
#define SECONDS_IN_A_DAY 86400L

static char utils_buff32[33];

char * formatDouble40(double);
char * formatDouble41(double);

#define formatInteger(x) itoa(x, utils_buff32, 10)

void delayMicros(uint32_t us);
void delayMs(uint32_t ms);

class TimeProvider {
private:
    static TimeProvider * _me;
protected:
    TimeProvider();
public:
    static TimeProvider * me();

    static void setProvider(TimeProvider * p);

    virtual const char * secondsToString(unsigned long);
    virtual const char * getUpTime();
    virtual const char * getTimeStringShort();
    virtual const char * getTimeStringLong();
    virtual unsigned long getSecondsOfDay();
    virtual void updateTime();
};

class ElapsedMillis {
private:
    unsigned long ms;
public:
    ElapsedMillis(void) {
        ms = millis();
    }
    operator unsigned long() const {
        return millis() - ms;
    }
};


#define setupLed(x) pinMode(x,OUTPUT); ledOff(x)
#define ledOn(x) digitalWrite(x,HIGH)
#define ledOff(x) digitalWrite(x,LOW)
#define ledPulse(x, y) ledOn(x); delayMs(2 * y / 3); ledOff(x); delayMs(y / 3)
#define ledSet(x,y) digitalWrite(x,y)


#endif
