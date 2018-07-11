#include "../Config.h"
#ifdef WATER_ON

#include "WaterMeter.hpp"
#include "../Sys/Utils.h"


WaterMeter::WaterMeter() : cold(WATER_COLD_PULSES_PER_LITER), hot(WATER_HOT_PULSES_PER_LITER) {}
WaterMeter::~WaterMeter() {}

WaterMeter * WaterMeter::_me;

WaterMeter * WaterMeter::me() {
    if (_me == NULL) {
        _me = new WaterMeter();
    }
    return _me;
};

void WaterMeter::updateData() {
    hot.rollUpPulses(); // collect pulses from ISR
    cold.rollUpPulses();
    calcWater();
}

void WaterMeter::calcWater() {
    hot.calcUnits();
    cold.calcUnits();
    hot.updateAverageFlow();
    cold.updateAverageFlow();
}

void WaterMeter::clearKept() {
    hot.clearKept();
    cold.clearKept();
    secondsKept = 0;
};

void WaterMeter::onColdPulseISR() {
    if (digitalRead(WATER_COLD_PULSE_PIN) == LOW) {
        me()->cold.debouncedPulseISR();
    }
}

void WaterMeter::onHotPulseISR() {
    if (digitalRead(WATER_HOT_PULSE_PIN) == LOW) {
        me()->hot.debouncedPulseISR();
    }
}

void WaterMeter::setPPU(double ppuCold, double ppuHot){
    cold.setPPU(ppuCold);
    hot.setPPU(ppuHot);
}

void WaterMeter::setup() {
    pinMode(WATER_HOT_PULSE_PIN, INPUT);
    attachInterrupt(WATER_HOT_PULSE_PIN, onHotPulseISR, FALLING);
    pinMode(WATER_COLD_PULSE_PIN, INPUT);
    attachInterrupt(WATER_COLD_PULSE_PIN, onColdPulseISR, FALLING);
}

void WaterMeter::loop() {
    static ElapsedMillis elapsed;
    if (elapsed > PULSE_DATA_ROLLUP_PERIOD_MS) {
        updateData();
        elapsed.rearm();
    }
}

const char * WaterMeter::getDataJson(unsigned int period_s) {
    secondsKept += period_s;
    snprintf(utils_buff64, 64, "{\"p_cold\":%d,\"p_hot\":%d,\"seconds\":%d}", cold.pulsesKept, hot.pulsesKept, secondsKept);
    return utils_buff64;
}

#endif //WATER_ON
