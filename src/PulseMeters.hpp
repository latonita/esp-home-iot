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
#include <RunningAverage.h>
#include <stdio.h>

#define PULSE_DATA_ROLLUP_PERIOD_MS 2000ul
#define PULSE_DEBOUNCE 100L // 100ms = 0.1s

struct Pulses {
public:
  Pulses(double ppu);
  ~Pulses();

  volatile unsigned long count = 0;
  volatile unsigned long lastMillis = 0;
  volatile unsigned long pulseWidth = 0;
  void debouncedPulseISR();

  unsigned int pulsesLast = 0;
  unsigned int pulsesKept = 0;
  void clearKept();

  unsigned int pulsesToday = 0;
  unsigned int pulsesYesterday = 0;
  unsigned long lastRollUp = 0;
  void rollUpPulses();

  double unitsKept = 0;
  double unitsConsumedToday = 0;
  double unitsConsumedYesterday = 0;
  double instantFlow = 0;

  void calcUnits();
  double calcFlow();
  void updateAverageFlow();
  RunningAverage averageFlow;
  void setPPU(double ppu);
  double pulsesPerUnit;
};

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
