//
//  PowerPulse.hpp
//  esp-home-iot
//
//  Created by <author> on 08/05/2017.
//
//

#ifndef PulseMeters_hpp
#define PulseMeters_hpp

#include "config.h"
#include <Arduino.h>
//#include <RunningAverage.h>
#include <stdio.h>
#include "Pulses.hpp"

// #define PULSE_DATA_ROLLUP_PERIOD_MS 2000ul
// #define PULSE_DEBOUNCE 100L // 100ms = 0.1s


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

  const char *formattedInstantPowerW(bool average); // todo remove
  const char *getDataJson(unsigned int period_s);

protected:
  PowerMeter();
  ~PowerMeter();
};

/*
class WaterMeter {
private:
  static WaterMeter *_me;
  Pulses cold;
  Pulses hot;

  static WaterMeter *me();
  static void onColdPulseISR();
  static void onHotPulseISR();

  void setup();
  void loop();
  void setPPU(double ppuCold, double ppuHot);
  void updateData();
  void clearKept();

  void calcWater();

protected:
  WaterMeter();
  ~WaterMeter();
};
*/
#endif
