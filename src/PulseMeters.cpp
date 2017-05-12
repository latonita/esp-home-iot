//
//  PowerPulse.cpp
//  esp-home-iot
//
//  Created by <author> on 08/05/2017.
//
//

#include "PulseMeters.hpp"
#include "utils.h"
#include <ArduinoJson.h>

Pulses::Pulses(double ppu) : pulsesPerUnit(ppu){}
Pulses::~Pulses(){}

void Pulses::debouncedPulseISR() {
    unsigned long now = micros();
    if (now >= lastMicros + PULSE_DEBOUNCE) {
        ++count;

        pulseWidth = now - lastMicros;
        lastMicros = now;
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

    //secondsKept += MQTT_DATA_COLLECTION_PERIOD_SECS;

    // add up today, and check y-day
    unsigned long now = TimeProvider::me()->getSecondsOfDay();
    if (lastRollUp > now) {
        //new day
        pulsesYesterday = pulsesToday;
        pulsesToday = 0;
    }
    lastRollUp = now;
}

void Pulses::clearKept() {
    pulsesKept = 0;
};

void Pulses::calcUnits() {
    unitsKept = pulsesKept / pulsesPerUnit;
    unitsConsumedToday = pulsesToday / pulsesPerUnit;
    unitsConsumedYesterday = pulsesYesterday / pulsesPerUnit;
    if (pulseWidth > 0) {
        instantFlow = (3600000000.0 / pulseWidth) * pulsesPerUnit;
    } else {
        instantFlow = 0;
    }
}

void Pulses::setPPU(double ppu) {
    pulsesPerUnit = ppu;
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
    calcEnergyAndPower();
}

const char * PowerMeter::getDataJson() {
    DynamicJsonBuffer json;
    JsonObject & root = json.createObject();
    root["pulses"] = pulse.pulsesKept;
    root["energy"] = String(pulse.unitsKept);
    root["power"] = String(instantWatts());
    root["eToday"] = String(energyConsumedTodayKWH());
    root["eYesterday"] = String(energyConsumedYesterdayKWH());

    static char dataBuffer[128];
    root.printTo(dataBuffer);
    return dataBuffer;
}

void PowerMeter::clearKept() {
    pulse.clearKept();
    secondsKept = 0;
};

void PowerMeter::onPowerPulseISR() {
    if (digitalRead(POWER_PULSE_PIN) == LOW) {
        me()->pulse.debouncedPulseISR();
    }
}

void PowerMeter::setup() {
    pinMode(POWER_PULSE_PIN, INPUT);
    attachInterrupt(POWER_PULSE_PIN, onPowerPulseISR, FALLING);
}

void PowerMeter::loop() {
    static ElapsedMillis elapsed;
    if (elapsed > PULSE_DATA_ROLLUP_PERIOD_MS) {
        updateData();
    }
}

void PowerMeter::setPPU(double ppu) {
    pulse.setPPU(ppu);
}

void PowerMeter::calcEnergyAndPower() {
    pulse.calcUnits();
}

double PowerMeter::instantWatts() {
    if (pulse.lastMicros > 0) {
        instantFlow = (3600000000.0 / (micros() - pulse.lastMicros)) * pulse.pulsesPerUnit;
    }
    return instantFlow;
}

const char * PowerMeter::formattedInstantPowerW() {
  #define FIP_BUF_LEN 10
    static char formattedInstantPower[FIP_BUF_LEN + 1];
    double watts = instantWatts();
    if (watts < 10) {
        snprintf(formattedInstantPower, FIP_BUF_LEN, ("-- W"));
    } else if (watts < 1000) {
        snprintf(formattedInstantPower, FIP_BUF_LEN, ("%s W"), formatDouble41(watts));
    } else if (watts < 1000000) {
        snprintf(formattedInstantPower, FIP_BUF_LEN, ("%s kW"), formatDouble41(watts / 1000));
    } else {
        snprintf(formattedInstantPower, FIP_BUF_LEN, ("9999.9 kW"));
    }
    return formattedInstantPower;
}

WaterMeter::WaterMeter() : cold(WATER_COLD_PULSES_PER_LITERS), hot(WATER_HOT_PULSES_PER_LITERS) {}
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
    }
}
