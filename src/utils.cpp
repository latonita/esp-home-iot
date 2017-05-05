#ifndef _LATONITA_UTILS_C_
#define _LATONITA_UTILS_C_

#include "utils.h"

char * secondsToString(unsigned long t) {
    static char str[12];
    long h = t / 3600;
    t = t % 3600;
    int m = t / 60;
    int s = t % 60;
    sprintf(str, "%04ld:%02d:%02d", h, m, s);
    return str;
}

char * getUpTime() {
    static unsigned long last_millis = 0;
    static unsigned char overflow = 0;
    unsigned long now = millis();
    if (now < last_millis) {
        overflow++;
    }
    last_millis = now;
    return secondsToString((now / 1000) + (MAX_UPTIME / 1000) * overflow);
}

void delayMicros(uint32_t us){
    uint32_t start = micros();
    while(micros() - start < us) {}
}

void delayMs(uint32_t ms){
    uint32_t start = millis();
    while(millis() - start < ms) {
        yield();
    }
}

char * formatDouble41(double d) {
    return dtostrf(d, 4, 1, utils_buff32);
}

#endif
