#ifndef RunningAverage_h
#define RunningAverage_h
//
//    FILE: RunningAverage.h
//  AUTHOR: Rob dot Tillaart at gmail dot com
// PURPOSE: RunningAverage library for Arduino
// VERSION: 0.1.01
//     URL: http://arduino.cc/playground/Main/RunningAverage
// HISTORY: See RunningAverage.cpp
//
// Released to the public domain
//

#include <stdio.h>
#include <inttypes.h>

class RunningAverage {
public:
    RunningAverage(uint8_t);
    ~RunningAverage();
    void    clr();
    void    add(double);
    double   avg();
    uint8_t count();
    double   peek();

protected:
    uint8_t _size;
    uint8_t _count;
    uint8_t _idx;
    double _sum;
    double * _ar;
};

#endif
