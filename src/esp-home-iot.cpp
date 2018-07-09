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
#include "EspNodeBase.hpp"
#include "Utils.h"
#include "ver.h"

#ifdef DISPLAY_ON
  #include "DisplayOn.h"
#endif 

#ifndef DISPLAY_ON
  #define drawProgress()
  #define drawEndlessProgress()
#endif

#ifdef POWER_ON
  #include "PulseMeters.hpp"
  PowerMeter *power = PowerMeter::me();
#endif

#ifdef DHT_ON
  #include "TemperatureSensor.hpp"
  TemperatureSensor dht(DHT_PIN);
  bool readyForDHTUpdate = false;
#endif

#ifdef DOORBELL_ON
  #include "DoorBell.hpp"
  bool someoneAtTheDoor = false;
#endif

bool readyToPublishData = false;
bool readyForDataUpdate = false;

void subscribeToMqttTopics() {
  Serial.println("subscribeToMqttTopics()");
  EspNodeBase::me()->mqttSubscribe("weather");
  EspNodeBase::me()->mqttSubscribe("weather-t");
  EspNodeBase::me()->mqttSubscribe("readings");
}

void unSubscribeFromMqttTopics() {
  Serial.println("unSubscribeFromMqttTopics()");
  EspNodeBase::me()->mqttUnsubscribe("weather");
  EspNodeBase::me()->mqttUnsubscribe("weather-t");
  EspNodeBase::me()->mqttUnsubscribe("readings");
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

  DisplayOn::drawProgress(DisplayOn::display, 10, "Updating time...");
  EspNodeBase::me()->getTimeProvider()->updateTime();
  ledPulse(LED2, 250);

#ifdef DHT_ON
  DisplayOn::drawProgress(DisplayOn::display, 50, "Updating DHT Sensor");
  dht.update(&readyForDHTUpdate);
  ledPulse(LED2, 250);
#endif

  DisplayOn::drawProgress(DisplayOn::display, 100, "Done...");
  ledPulse(LED2, 250);
}

void publishData() {
  Serial.println("Publishing data ...");
  readyToPublishData = false;
  ledSet(LED2, HIGH);

#ifdef POWER_ON
  power->updateData();
  if (EspNodeBase::me()->mqttPublish("power", power->getDataJson(MQTT_DATA_COLLECTION_PERIOD_SECS), false))
    power->clearKept();
  else
    Serial.println("MQTT publish fail");
#endif

#ifdef DHT_ON
  EspNodeBase::me()->mqttPublish("temperature", dht.getDataJson(), true);
#endif

  Serial.println("Publishing data finished.");
  ledSet(LED2, LOW);
}

char *readingsRaw = NULL;
void parseDelimetedString(char *buf, char **ptrs, unsigned int max,
                          const char *raw, unsigned int len) {
  if (len + 1 > BUF_MAX) {
    Serial.printf("parseDelimetedString: buff %d < len %d\r\n", BUF_MAX, len + 1);
    return;
  }

  if (len > 0) {
    unsigned int i = 0;
    for (i = 0; i < max; ++i)
      ptrs[i] = NULL;
    memset(buf, 0, BUF_MAX);
    strncpy(buf, raw, len);
    buf[len] = 0;

    i = 0;
    char *p = strtok(buf, ";");
    while (p != NULL && i < max) {
      ptrs[i++] = p;
      p = strtok(NULL, ";");
    }
  }
}

void onMqttEvent(EspNodeBase::MqttEvent event, const char *topic, const char *message, unsigned int len) {
  switch (event) {
  case EspNodeBase::MqttEvent::CONNECT:
    Serial.println("onMqttEvent::CONNECT");
    DisplayOn::drawEndlessProgress((char *)F("Connecting to MQTT"), true);
    subscribeToMqttTopics();
    break;
  case EspNodeBase::MqttEvent::DISCONNECT:
    Serial.println("onMqttEvent::DISCONNECT");
    DisplayOn::drawEndlessProgress((char *)F("Connecting to MQTT"));
    break;
  case EspNodeBase::MqttEvent::MESSAGE:
    Serial.println("onMqttEvent::MESSAGE");
    if (strcmp(topic, "readings") == 0)
      parseDelimetedString(DisplayOn::bufPowerStats, DisplayOn::powerStats, POWER_PARAMS, message, len);
    else if (strcmp(topic, "weather") == 0)
      parseDelimetedString(DisplayOn::bufWeather, DisplayOn::weatherItems, WEATHER_DAYS * WEATHER_PARAMS, message, len);
    else if (strcmp(topic, "weather-t") == 0)
      parseDelimetedString(DisplayOn::bufWeatherText, DisplayOn::weatherText, WEATHER_DAYS, message, len);
    break;

  }
}

void setupNetworkHandlers() {
  EspNodeBase::me()->registerWifiHandler([](EspNodeBase::WifiEvent e, int counter) {
    if (e == EspNodeBase::WifiEvent::CONNECTING) {
      DisplayOn::drawEndlessProgress((char *)F("Connecting to WiFi"));
      ledPulse(LED2, 250);
    } else if (e == EspNodeBase::WifiEvent::FAILURE) {
      DisplayOn::drawEndlessProgress((char *)F("WiFi failure..."));
      ledPulse(LED2, 250);
    }
  });
  #ifdef DISPLAY_ON
    EspNodeBase::me()->registerOtaHandlers(
        []() {
          DisplayOn::display->clear();
          DisplayOn::display->setFont(ArialMT_Plain_10);
          DisplayOn::display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
          DisplayOn::display->drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, (char *)F("OTA Update"));
          DisplayOn::display->display();
        },
        []() {
          DisplayOn::display->clear();
          DisplayOn::display->setFont(ArialMT_Plain_10);
          DisplayOn::display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
          DisplayOn::display->drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, (char *)F("Restart"));
          DisplayOn::display->display();
          publishData(); // publish data before restart
        },
        [](unsigned int progress, unsigned int total) {
          int p = progress / (total / 100);
          DisplayOn::display->drawProgressBar(4, 32, 120, 8, p);
          DisplayOn::display->display();
          ledSet(LED2, (p & 1) ? LOW : HIGH);
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
        ledSet(LED2, (p & 1) ? LOW : HIGH);
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
  #ifdef POWER_ON
    //setup ISR first to minimize lost pulses during reset/power on
    power->setup();
  #endif

  Serial.begin(SERIAL_BAUD_RATE);
  delayMs(50);
  Serial.println("Entering setup.");
  delayMs(50);
  
  EspNodeBase::me()->setup();
  
  setupLed(LED1);
  setupLed(LED2);

  #ifdef DISPLAY_ON
    DisplayOn::initDisplayAndUI();
  #endif

  setupNetworkHandlers();
  EspNodeBase::me()->startNetworkStack();

  setupRegularActions();
  setupDoorBell();

  updateData();

  Serial.println("Setup finished.");
}

void loop() {
  EspNodeBase::me()->loop();
  yield();
  
#ifdef POWER_ON
  ledSet(LED1, digitalRead(POWER_PULSE_PIN));
  power->loop();
  #ifdef DISPLAY_ON
    DisplayOn::formattedInstantPower = power->formattedInstantPowerW(true);
  #endif
#endif

#ifdef DOORBELL_ON
  DoorBell::loop();
#endif 

#ifdef DISPLAY_ON
  if (DisplayOn::isFixed()) {
#endif    
    if (someoneAtTheDoor && EspNodeBase::me()->isConnected()) {
      Serial.println("UI is still. Let's publish door bell");
      publishDoorBell();
    }
#ifdef DISPLAY_ON
  }
#endif

#ifdef DISPLAY_ON
  if (DisplayOn::isFixed()) {
#endif    
    if (readyToPublishData && EspNodeBase::me()->isConnected()) {
      Serial.println("UI is still. Let's publish data");
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
