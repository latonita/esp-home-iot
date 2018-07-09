//
//  PowerPulse.cpp
//  esp-home-iot
//
//  Created by <author> on 08/05/2017.
//
//

#include "PulseMeters.hpp"
#include "utils.h"

Pulses::Pulses(double ppu) : averageFlow(5), pulsesPerUnit(ppu) {}
Pulses::~Pulses(){}

void Pulses::debouncedPulseISR() {
    unsigned long now = millis();
    if (now >= lastMillis + PULSE_DEBOUNCE) {
        ++count;

        pulseWidth = now - lastMillis;
        lastMillis = now;
    }
}

void Pulses::rollUpPulses() {
    // collect from ISR
    cli();
    pulsesLast = count;
    count = 0;
    sei();

    pulsesKept += pulsesLast;
    pulsesToday += pulsesLast;
}

void Pulses::clearKept() {
    pulsesKept = 0;
};

void Pulses::calcUnits() {
    unitsKept = pulsesKept / pulsesPerUnit;
}

void Pulses::setPPU(double ppu) {
    pulsesPerUnit = ppu;
}

double Pulses::calcFlow() {
    unsigned long currentPulseWidth = millis() - lastMillis;

    if (currentPulseWidth < pulseWidth) {
        currentPulseWidth = pulseWidth;
    }

    if (currentPulseWidth > 0) {
        instantFlow = 3600000.0 / (currentPulseWidth * pulsesPerUnit);
    } else {
        instantFlow = 0;
    }
    return instantFlow;
}
void Pulses::updateAverageFlow() {
    averageFlow.add(calcFlow());
}

PowerMeter::PowerMeter() : pulse(POWER_PULSES_PER_WATT_HOUR) {}
PowerMeter::~PowerMeter() {}

PowerMeter * PowerMeter::_me;

PowerMeter * PowerMeter::me() {
    if (_me == NULL) {
        _me = new PowerMeter();
    }
    return _me;
};

void PowerMeter::updateData() {
    pulse.rollUpPulses();
}

const char * PowerMeter::getDataJson(unsigned int period_s) {
    secondsKept += period_s;
    snprintf(utils_buff64, 64, "{\"pulses\":%d,\"seconds\":%d,\"power\":%.2f}", pulse.pulsesKept, secondsKept,averageWatts());
    return utils_buff64;
}

void PowerMeter::clearKept() {
    pulse.clearKept();
    secondsKept = 0;
};

void PowerMeter::onPowerPulseISR() {
    if (digitalRead(POWER_PULSE_PIN) == HIGH) {
        me()->pulse.debouncedPulseISR();
    }
}

void PowerMeter::setup() {
    pinMode(POWER_PULSE_PIN, INPUT);
    attachInterrupt(POWER_PULSE_PIN, onPowerPulseISR, RISING);
}

void PowerMeter::loop() {
    static ElapsedMillis elapsed;
    if (elapsed > PULSE_DATA_ROLLUP_PERIOD_MS) {
        pulse.rollUpPulses();
        pulse.calcUnits();
        pulse.updateAverageFlow();
        elapsed.rearm();
    }
}

void PowerMeter::setPPU(double ppu) {
    pulse.setPPU(ppu);
}

double PowerMeter::averageWatts() {
    return pulse.averageFlow.avg();
}

double PowerMeter::instantWatts() {
    return pulse.instantFlow;
}

const char * PowerMeter::formattedInstantPowerW(bool average = false) {
  #define FIP_BUF_LEN 10
    static char formattedInstantPower[FIP_BUF_LEN + 1];
    double watts = average ? averageWatts() : instantWatts();
    if (watts < 10) {
        snprintf(formattedInstantPower, FIP_BUF_LEN, ("-- W"));
    } else if (watts < 1000) {
        snprintf(formattedInstantPower, FIP_BUF_LEN, ("%s W"), formatDouble40(watts));
    } else if (watts < 1000000) {
        snprintf(formattedInstantPower, FIP_BUF_LEN, ("%s kW"), formatDouble41(watts / 1000));
    } else {
        snprintf(formattedInstantPower, FIP_BUF_LEN, ("9999.9 kW"));
    }
    return formattedInstantPower;
}
/*
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

void WaterMeter::clearKept() {
    hot.clearKept();
    cold.clearKept();
    //secondsKept = 0;
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
*/