#include "../Config.h"
#ifdef DOORBELL_ON
#include "DoorBell.hpp"

std::function<void()> DoorBell::callback = NULL;
bool DoorBell::bell = false;

void DoorBell::setCallback(std::function<void()> fn) {
    callback = fn;
}

void DoorBell::setup() {
    bell = false;
    pinMode(DOORBELL_PIN, INPUT);
    attachInterrupt(DOORBELL_PIN, onDoorBellISR, FALLING);
}
void DoorBell::loop() {
    if (bell && callback != NULL) {
        DoorBell::callback();
    } 
    bell = false;
}

void DoorBell::onDoorBellISR() {
    static unsigned long lastMillis;
    if (digitalRead(DOORBELL_PIN) == LOW) {
    unsigned long now = millis();
    if (now >= lastMillis + DOORBELL_DEBOUNCE) {
        lastMillis = now;
        bell = true;
    }
}}

#endif //DOORBELL_ON