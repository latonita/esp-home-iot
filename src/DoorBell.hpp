#ifndef _DOOR_BELL_H_
#define _DOOR_BELL_H_

#include <Arduino.h>
#include "Config.h"

#define DOORBELL_DEBOUNCE 15 * 1000 // 15 seconds

class DoorBell {
  private: 
    static std::function<void()> callback;
    static bool bell;
    DoorBell(){};

  public: 
    static void setCallback(std::function<void()> fn);
    static void setup();
    static void loop();

  private:
    static void onDoorBellISR();
};

#endif 