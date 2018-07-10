#include "Config.h"
#ifdef WATER_ON

#ifndef WaterMeter_hpp
#define WaterMeter_hpp

#include <Arduino.h>
#include <stdio.h>
//#include <RunningAverage.h>
#include "Pulses.hpp"

class WaterMeter {
private:
  static WaterMeter *_me;
  Pulses cold;
  Pulses hot;
  unsigned int secondsKept = 0;

public:
  static WaterMeter *me();
  static void onColdPulseISR();
  static void onHotPulseISR();

  void setup();
  void loop();
  void setPPU(double ppuCold, double ppuHot);
  void updateData();
  void clearKept();

  void calcWater();

  const char *getDataJson(unsigned int period_s);

protected:
  WaterMeter();
  ~WaterMeter();
};

#endif

#endif