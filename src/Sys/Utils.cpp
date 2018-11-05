#ifndef _LATONITA_UTILS_C_
#define _LATONITA_UTILS_C_

#include "Utils.h"

char utils_buff32[33];
char utils_buff64[65];

void delayMicros(uint32_t us) {
    uint32_t start = micros();
    while (micros() - start < us) {
    }
}

void delayMs(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

const char *formatDouble40(double d) { return dtostrf(d, 4, 0, utils_buff32); }
const char *formatDouble41(double d) { return dtostrf(d, 4, 1, utils_buff32); }

TimeProvider::TimeProvider(){};

TimeProvider *TimeProvider::_me;
TimeProvider *TimeProvider::me() {
    if (_me == NULL) {
        _me = new TimeProvider();
    }
    return _me;
}

void TimeProvider::setProvider(TimeProvider *p) {
    if (_me != NULL) {
        delete _me;
    }
    _me = p;
}

TimeProvider::~TimeProvider() {}

const char *TimeProvider::secondsToString(unsigned long t) {
    static char str[12];
    long h = t / 3600;
    t = t % 3600;
    int m = t / 60;
    int s = t % 60;
    snprintf(str, 12, "%04ld:%02d:%02d", h, m, s);
    return str;
}

const char *TimeProvider::getUpTime() {
    static unsigned long last_millis = 0;
    static unsigned char overflow = 0;
    unsigned long now = millis();
    if (now < last_millis) {
        overflow++;
    }
    last_millis = now;
    return secondsToString((now / 1000) + (MAX_UPTIME / 1000) * overflow);
}

unsigned long TimeProvider::getSecondsOfDay() {
    // default dummy impl
    return (millis() / 1000) % 86400L;
}

const char *TimeProvider::getTimeStringShort() {
    static char str[12];
    auto t = getSecondsOfDay();
    long h = t / 3600;
    t = t % 3600;
    int m = t / 60;
    snprintf(str, 12, "%02ld:%02d", h, m);
    return str;
}
const char *TimeProvider::getTimeStringLong() {
    static char str[12];
    auto t = getSecondsOfDay();
    long h = t / 3600;
    t = t % 3600;
    int m = t / 60;
    int s = t % 60;
    snprintf(str, 12, "%02ld:%02d:%02d", h, m, s);
    return str;
}

const char *TimeProvider::getDateString() { return "-"; }

void TimeProvider::updateTime() { return; }

void parseDelimetedString(char *buf, const unsigned int bufSize, char **ptrs, unsigned int max, const char *raw, unsigned int len) {
    if (len + 1 > bufSize) {
        Serial.printf("parseDelimetedString: buff %d < len %d\r\n", bufSize, len + 1);
        return;
    }

    if (len > 0) {
        unsigned int i = 0;
        for (i = 0; i < max; ++i)
            ptrs[i] = NULL;
        // memset(buf, 0, BUF_MAX);
        // strncpy(buf, raw, len);
        auto newLen = len > bufSize ? bufSize : len;
        memcpy(buf, raw, newLen);
        buf[newLen] = '\0';

        i = 0;
        // /* following code leaks on esp8266 */
        // char *p = strtok(buf, ";");
        // while (p != NULL && i < max) {
        //   ptrs[i++] = p;
        //   p = strtok(NULL, ";");
        // }

        char *p = buf;
        while (*p != '\0' && i < max) {
            ptrs[i++] = p;
            while (*p != '\0' && *p != ';')
                p++;
            if (*p != '\0')
                *p++ = '\0';
        }
    }
}

#endif
