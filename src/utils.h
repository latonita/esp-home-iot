#ifndef _LATONITA_UTILS_H_
#define _LATONITA_UTILS_H_

#include <stdlib.h>
#include <stdint.h>
#include <Arduino.h>

//system setting
#define MAX_UPTIME 4294967295

static char utils_buff32[33];

char * secondsToString(unsigned long t);
char * getUpTime();
char * formatDouble41(double);

#define formatInteger(x) itoa(x, utils_buff32, 10)

void delayMicros(uint32_t us);
void delayMs(uint32_t ms);


#define setupLed(x) pinMode(x,OUTPUT); ledOff(x)
#define ledOn(x) digitalWrite(x,HIGH)
#define ledOff(x) digitalWrite(x,LOW)
#define ledPulse(x, y) ledOn(x); delayMs(2 * y / 3); ledOff(x); delayMs(y / 3)
#define ledSet(x,y) digitalWrite(x,y)


#endif
