#ifndef _LATONITA_UTILS_H_
#define _LATONITA_UTILS_H_

#include <stdlib.h>
#include <stdint.h>
#include <Arduino.h>

static char utils_buff32[33];

char * secondsToString(unsigned long t);
char * getUpTime();
char * formatDouble41(double);

#define formatInteger(x) itoa(x, utils_buff32, 10)

void delayMicros(uint32_t us);
void delayMs(uint32_t ms);

#endif
