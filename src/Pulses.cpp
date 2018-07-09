#include "Pulses.hpp"
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
