/*
   2017. Anton Viktorov,
   latonita@yandex.ru,
   http://github.com/latonita
   This work is based on Daniel Eichhorn's Weather Station Demo sketch .
   I used to have quite similar application, but lost its source code.
   So I took Daniel's work as a base and reworked it a lot.
   Daniel's page: http://blog.squix.ch

   My orignal work was based on MIT, and this one is not an exception.
*/
/*
   The MIT License (MIT)

   Copyright (c) 2017 by Anton Viktorov
   Copyright (c) 2016 by Daniel Eichhorn

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.


   // Please read http://blog.squix.org/weatherstation-getting-code-adapting-it
   // for setup instructions
 */
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

#include "Config.h"
#include "Sys/EspNodeBase.hpp"
#include "Sys/Utils.h"

#ifdef DISPLAY_ON
  #include "DisplayOn.h"
#endif 

#ifndef DISPLAY_ON
  #define drawProgress()
  #define drawEndlessProgress()
#endif

#ifdef POWER_ON
  #include "Sensors/PowerMeter.hpp"
  PowerMeter *power = PowerMeter::me();
#endif

#ifdef WATER_ON
  #include "Sensors/WaterMeter.hpp"
#endif

#ifdef DHT_ON
  #include "Sensors/TemperatureSensor.hpp"
  TemperatureSensor dht(DHT_PIN);
  bool readyForDHTUpdate = false;
#endif

#ifdef DOORBELL_ON
  #include "Sensors/DoorBell.hpp"
  bool someoneAtTheDoor = false;
#endif

bool readyToPublishData = false;
bool readyForDataUpdate = false;

const char* topicsOfInterest[] = { ""
  #ifdef DISPLAY_ON
    ,"weather", "weather-t", "readings"
  #endif
};
int topicsOfInterestNum = (sizeof(topicsOfInterest) / sizeof(topicsOfInterest[0])) - 1;

void unSubscribeFromMqttTopics() {
  Serial.println("unSubscribeFromMqttTopics()");
  for (int i = 1; i <= topicsOfInterestNum; i++) {
    EspNodeBase::me()->mqttUnsubscribe(topicsOfInterest[i]);
  }
}

void subscribeToMqttTopics() {
  Serial.println("subscribeToMqttTopics()");
  for (int i = 1; i <= topicsOfInterestNum; i++) {
    EspNodeBase::me()->mqttSubscribe(topicsOfInterest[i]);
  }
}

void setupRegularActions() {
  EspNodeBase::me()->addRegularAction(MQTT_DATA_COLLECTION_PERIOD_SECS, [](){
    Serial.println("Ticker: readyToPublishData");
    readyToPublishData = true;
  });
  EspNodeBase::me()->addRegularAction(MQTT_RESUBSCRIBE_SECS, [](){
    unSubscribeFromMqttTopics();
    subscribeToMqttTopics();
  });

  #ifdef DHT_ON
    EspNodeBase::me()->addRegularAction(DHT_UPDATE_INTERVAL_SECS, [](){
      Serial.println("Ticker: readyForDHTUpdate");
      readyForDHTUpdate = true;
    });
  #endif
}

void updateData() {
  readyForDataUpdate = false;
  Serial.println("Data update...");
  #ifdef DISPLAY_ON
    DisplayOn::drawProgress(DisplayOn::display, 10, "Updating time...");
  #endif 
  EspNodeBase::me()->getTimeProvider()->updateTime();
  ledPulse(LED_INFO, 250);

#ifdef DHT_ON
  DisplayOn::drawProgress(DisplayOn::display, 50, "Updating DHT Sensor");
  dht.update(&readyForDHTUpdate);
  ledPulse(LED_INFO, 250);
#endif

  #ifdef DISPLAY_ON
    DisplayOn::drawProgress(DisplayOn::display, 100, "Done...");
  #endif 
  ledPulse(LED_INFO, 250);
}

void publishData() {
  Serial.println("Publishing data ...");
  readyToPublishData = false;
  ledSet(LED_INFO, HIGH);

#ifdef POWER_ON
  power->updateData();
  if (EspNodeBase::me()->mqttPublish("power", power->getDataJson(MQTT_DATA_COLLECTION_PERIOD_SECS), false))
    power->clearKept();
  else
    Serial.println("MQTT publish fail");
#endif

#ifdef WATER_ON
  WaterMeter::me()->updateData();
  if (EspNodeBase::me()->mqttPublish("water", WaterMeter::me()->getDataJson(MQTT_DATA_COLLECTION_PERIOD_SECS), false))
    WaterMeter::me()->clearKept();
  else
    Serial.println("MQTT publish fail");
#endif

#ifdef DHT_ON
  EspNodeBase::me()->mqttPublish("temperature", dht.getDataJson(), true);
#endif

  Serial.println("Publishing data finished.");
  ledSet(LED_INFO, LOW);
}

char *readingsRaw = NULL;

void onMqttEvent(EspNodeBase::MqttEvent event, const char *topic, const char *message, unsigned int len) {
  switch (event) {
  case EspNodeBase::MqttEvent::CONNECT:
    Serial.println("onMqttEvent::CONNECT");
    #ifdef DISPLAY_ON
      DisplayOn::drawEndlessProgress("Connecting to MQTT", true);
    #endif 
    subscribeToMqttTopics();
    break;
  case EspNodeBase::MqttEvent::DISCONNECT:
    Serial.println("onMqttEvent::DISCONNECT");
    #ifdef DISPLAY_ON
      DisplayOn::drawEndlessProgress("Connecting to MQTT");
    #endif 
    break;
  case EspNodeBase::MqttEvent::MESSAGE:
    Serial.println("onMqttEvent::MESSAGE");
    #ifdef DISPLAY_ON
      if (strcmp(topic, "readings") == 0)
        parseDelimetedString(DisplayOn::bufPowerStats, DisplayOn::powerStats, POWER_PARAMS, message, len);
      else if (strcmp(topic, "weather") == 0)
        parseDelimetedString(DisplayOn::bufWeather, DisplayOn::weatherItems, WEATHER_DAYS * WEATHER_PARAMS, message, len);
      else if (strcmp(topic, "weather-t") == 0)
        parseDelimetedString(DisplayOn::bufWeatherText, DisplayOn::weatherText, WEATHER_DAYS, message, len);
    #endif
    break;

  }
}

void setupNetworkHandlers() {
  EspNodeBase::me()->registerWifiHandler([](EspNodeBase::WifiEvent e, int counter) {
    if (e == EspNodeBase::WifiEvent::CONNECTING) {
      #ifdef DISPLAY_ON
        DisplayOn::drawEndlessProgress("Connecting to WiFi");
      #endif
      ledPulse(LED_INFO, 250);
    } else if (e == EspNodeBase::WifiEvent::FAILURE) {
      #ifdef DISPLAY_ON
        DisplayOn::drawEndlessProgress("WiFi failure...");
      #endif
      ledPulse(LED_INFO, 250);
    }
  });
  #ifdef DISPLAY_ON
    EspNodeBase::me()->registerOtaHandlers(
        []() {
          DisplayOn::display->clear();
          DisplayOn::display->setFont(ArialMT_Plain_10);
          DisplayOn::display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
          DisplayOn::display->drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, "OTA Update");
          DisplayOn::display->display();
        },
        []() {
          DisplayOn::display->clear();
          DisplayOn::display->setFont(ArialMT_Plain_10);
          DisplayOn::display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
          DisplayOn::display->drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "Restart");
          DisplayOn::display->display();
          publishData(); // publish data before restart
        },
        [](unsigned int progress, unsigned int total) {
          int p = progress / (total / 100);
          DisplayOn::display->drawProgressBar(4, 32, 120, 8, p);
          DisplayOn::display->display();
          ledSet(LED_INFO, (p & 1) ? LOW : HIGH);
        },
        NULL);
  #else
  EspNodeBase::me()->registerOtaHandlers(
      NULL,
      []() { 
        publishData();
      },
      [](unsigned int progress, unsigned int total) {
        int p = progress / (total / 100);
        ledSet(LED_INFO, (p & 1) ? LOW : HIGH);
      },
      NULL);
  #endif
  EspNodeBase::me()->addMqttEventHandler(onMqttEvent);
}

#ifdef DOORBELL_ON
  void publishDoorBell() {
    Serial.println("Publishing door bell ...");
    if (EspNodeBase::me()->mqttPublish("doorbell", "true", false)) {
      someoneAtTheDoor = false;
    } else {
      Serial.println("MQTT publish fail");
    }
  }

  void setupDoorBell() {
    DoorBell::setup();
    DoorBell::setCallback([](){
      Serial.println("Door bell, someone at the door!");
      someoneAtTheDoor = true;
    });
  }
#endif //DOORBELL_ON


void setup() {
    //setup ISRs first to minimize lost pulses during reset/power on
  #ifdef POWER_ON
    power->setup();
    setupLed(LED_POWER_PULSE);
  #endif
  #ifdef WATER_ON
    WaterMeter::me()->setup();
  #endif

  Serial.begin(SERIAL_BAUD_RATE);
  delayMs(50);
  Serial.println("Entering setup.");
  delayMs(50);

  setupLed(LED_INFO);
  EspNodeBase::me()->setup();

  #ifdef DISPLAY_ON
    DisplayOn::initDisplayAndUI();
  #endif

  setupNetworkHandlers();
  EspNodeBase::me()->startNetworkStack();

  setupRegularActions();
  
  #ifdef DOORBELL_ON
    setupDoorBell();
  #endif

  updateData();

  Serial.println("Setup finished.");
}

void loop() {
  EspNodeBase::me()->loop();
  yield();
  
#ifdef POWER_ON
  ledSet(LED_POWER_PULSE, digitalRead(POWER_PULSE_PIN));
  power->loop();
  #ifdef DISPLAY_ON
    DisplayOn::formattedInstantPower = power->formattedInstantPowerW(true);
  #endif
#endif

#ifdef DOORBELL_ON
  DoorBell::loop();
  #ifdef DISPLAY_ON
    if (DisplayOn::isFixed()) {
  #endif    
      if (someoneAtTheDoor && EspNodeBase::me()->isConnected()) {
        Serial.println("Let's publish door bell");
        publishDoorBell();
      }
  #ifdef DISPLAY_ON
    }
  #endif
#endif

#ifdef DISPLAY_ON
  if (DisplayOn::isFixed()) {
#endif    
    if (readyToPublishData && EspNodeBase::me()->isConnected()) {
      Serial.println("Let's publish sensor data");
      publishData();
#ifdef DISPLAY_ON
  }
#endif

#ifdef DHT_ON
    readyForDataUpdate |= readyForDHTUpdate;
#endif

    if (readyForDataUpdate)
      updateData();
  }

#ifdef DISPLAY_ON
  int remainingTimeBudget = DisplayOn::ui->update();
  if (remainingTimeBudget > 0) {
    delayMs(remainingTimeBudget);
  }
#endif
  yield();
}
