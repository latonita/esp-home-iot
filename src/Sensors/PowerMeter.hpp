#include "../Config.h"
#ifdef POWER_ON

#ifndef PowerMeter_hpp
#define PowerMeter_hpp

#include <Arduino.h>
#include <stdio.h>
#include "Pulses.hpp"

const char *formattedInstantPowerW(double watts);

class PowerMeter {
private:
  static PowerMeter *_me;

public:
  Pulses pulse;
  double energyConsumedTodayKWH() { return pulse.unitsConsumedToday / 1000; }
  double energyConsumedYesterdayKWH() {
    return pulse.unitsConsumedYesterday / 1000;
  }

  double instantWatts();
  double averageWatts();

  unsigned int secondsKept = 0;

  static PowerMeter *me();
  static void onPowerPulseISR();

  void setup();
  void loop();
  void setPPU(double ppu);
  void updateData();
  void clearKept();

  static const char *formattedInstantPowerW(double watts);
  const char *formattedInstantPowerW(bool average);
  const char *getDataJson(unsigned int period_s);

protected:
  PowerMeter();
  ~PowerMeter();
};

#endif

#endif