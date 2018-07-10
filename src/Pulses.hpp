#ifndef Pulses_hpp
#define Pulses_hpp

#include <Arduino.h>
#include <stdio.h>
#include <RunningAverage.h>
#include "Config.h"

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

#endif