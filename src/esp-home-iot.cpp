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
#include "EspNodeBase.hpp"

#include <Arduino.h>

#include <OLEDDisplay.h>
#include <OLEDDisplayUi.h>
#include <SSD1306Wire.h>
#include <Wire.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#include <ArduinoOTA.h>  //For OTA
#include <ESP8266mDNS.h> //For OTA
#include <PubSubClient.h>
#include <WiFiUdp.h> //For OTA

#include "res/LatonitaIcons.h"
#include "res/WeatherStationFonts.h"
#include "res/WeatherStationImages.h"

#include "config.h"
#include "utils.h"
#include "ver.h"

#ifdef DHT_ON
#include "dht_util.hpp"
#endif

#ifdef POWER_ON
#include "PulseMeters.hpp"
#endif

#ifdef DOORBELL_ON
#include "DoorBell.hpp"
#endif

EspNodeBase *me = EspNodeBase::me();

/***************************
 * DHT
 **************************/
#ifdef DHT_ON
dht_util dht(DHT_PIN);
#endif

/***************************
 * POWER PULSES
 **************************/
#ifdef POWER_ON
PowerMeter *power = PowerMeter::me();
#endif

/***************************
 * ThingSpeak Settings
 **************************/
#ifdef THINGSPEAK_ON
// Thingspeak Settings
const String THINGSPEAK_CHANNEL_ID = "67284";
const String THINGSPEAK_API_READ_KEY = "L2VIW20QVNZJBLAK";
ThingspeakClient thingspeak;
#endif

/***************************
 * display objects
 **************************/
SSD1306Wire *display;
OLEDDisplayUi *ui;

/***************************
 * Ticker and flags
 **************************/

bool readyToPublishData = false;
void setReadyToPublishData();

bool readyForDataUpdate = false;

#ifdef DHT_ON
bool readyForDHTUpdate = false;
void setReadyForDHTUpdate();
#endif

#define BUF_MAX 255

#ifdef POWER_ON
#define POWER_P_T1 0
#define POWER_P_T2 1
#define POWER_P_YESTERDAY 2
#define POWER_P_TODAY 3
#define POWER_PARAMS 4
const char *formattedInstantPower = "";
char *bufPowerStats = new char[BUF_MAX]; // NULL;
char *powerStats[POWER_PARAMS] = {NULL, NULL, NULL, NULL};
#endif

#define WEATHER_DAYS 4
#define WEATHER_P_DAY 0
#define WEATHER_P_TEMP 1
#define WEATHER_P_ICON 2
#define WEATHER_PARAMS 3
char *bufWeather = new char[BUF_MAX]; // NULL;
char *weatherItems[WEATHER_DAYS * WEATHER_PARAMS] = {
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL};  // 4 days x 4 params
char *bufWeatherText = new char[BUF_MAX]; // NULL;
char *weatherText[WEATHER_DAYS] = {NULL, NULL, NULL, NULL};

/***************************
 * forward declarations
 **************************/

void updateData();

void drawProgress(OLEDDisplay *display, int percentage, String label);

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x,
                  int16_t y);
#ifdef POWER_ON
void drawInstantPower(OLEDDisplay *display, OLEDDisplayUiState *state,
                      int16_t x, int16_t y);
void drawEnergyConsumption(OLEDDisplay *display, OLEDDisplayUiState *state,
                           int16_t x, int16_t y);
void drawEnergyReadings(OLEDDisplay *display, OLEDDisplayUiState *state,
                        int16_t x, int16_t y);
#endif
#ifdef WEATHER_ON
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState *state,
                        int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x,
                  int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
#endif
#ifdef DHT_ON
void drawIndoor(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x,
                int16_t y);
#endif
#ifdef THINGSPEAK_ON
void drawThingspeak(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x,
                    int16_t y);
#endif
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state);

// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = {drawDateTime
#ifdef POWER_ON
                          ,
                          drawInstantPower, drawEnergyConsumption,
                          drawEnergyReadings
#endif
#ifdef WEATHER_ON
                          ,
                          drawCurrentWeather, drawForecast
#endif
#ifdef DHT_ON
//                           , drawIndoor
#endif
#ifdef THINGSPEAK_ON
                          ,
                          drawThingspeak
#endif
};
int numberOfFrames = (sizeof(frames) / sizeof(FrameCallback));
OverlayCallback overlays[] = {drawHeaderOverlay};
int numberOfOverlays = 1;

void subscribeToMqttTopics() {
  Serial.println("subscribeToMqttTopics()");
  me->mqttSubscribe("weather");
  me->mqttSubscribe("weather-t");
  me->mqttSubscribe("readings");
}

void resubscribeToMqttTopics() {
  Serial.println("resubscribeToMqttTopics");
  me->mqttUnsubscribe("weather");
  me->mqttUnsubscribe("weather-t");
  me->mqttUnsubscribe("readings");
  subscribeToMqttTopics();
}

void setupRegularActions() {
  me->addRegularAction(MQTT_DATA_COLLECTION_PERIOD_SECS, setReadyToPublishData);
  me->addRegularAction(MQTT_RESUBSCRIBE_SECS, resubscribeToMqttTopics);

#ifdef DHT_ON
  me->addRegularAction(DHT_UPDATE_INTERVAL_SECS, setReadyForDHTUpdate);
#endif
}

void initDisplay() {
  Serial.println(F("OLED hardware init..."));
  display = new SSD1306Wire(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
  display->init();
  display->clear();
  display->display();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setContrast(180);
}

void initUi() {
  Serial.println(F("OLED UI Init..."));
  ui = new OLEDDisplayUi(display);
  ui->setTargetFPS(30);
  // ui->setActiveSymbol(activeSymbole);
  // ui->setInactiveSymbol(inactiveSymbole);
  ui->setActiveSymbol(emptySymbol);   // Hack until disableIndicator works
  ui->setInactiveSymbol(emptySymbol); // - Set an empty symbol
  ui->disableIndicator();

  // You can change this to TOP, LEFT, BOTTOM, RIGHT
  ui->setIndicatorPosition(BOTTOM);
  // Defines where the first frame is located in the bar.
  ui->setIndicatorDirection(LEFT_RIGHT);
  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui->setFrameAnimation(SLIDE_LEFT);
  ui->setFrames(frames, numberOfFrames);
  ui->setOverlays(overlays, numberOfOverlays);
  // Inital UI takes care of initalising the display too.
  ui->init();

  updateData();
}

void drawEndlessProgress(const char *msg, bool finished = false) {
  static int counter = 0;
  display->clear();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->drawString(DISPLAY_WIDTH / 2, 10, msg);
  display->drawXbm(46, 30, 8, 8, finished || (counter % 3 == 0)
                                     ? activeSymbole
                                     : inactiveSymbole);
  display->drawXbm(60, 30, 8, 8, finished || (counter % 3 == 1)
                                     ? activeSymbole
                                     : inactiveSymbole);
  display->drawXbm(74, 30, 8, 8, finished || (counter % 3 == 2)
                                     ? activeSymbole
                                     : inactiveSymbole);
  if (finished)
    display->drawString(DISPLAY_WIDTH / 2, 50, "Done");
  display->display();
  delayMs(finished ? 500 : 100);
  counter++;
}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void updateData() {
  readyForDataUpdate = false;
  Serial.println("Data update...");
  drawProgress(display, 10, "Updating time...");
  me->getTimeProvider()->updateTime();
  ledPulse(LED2, 250);

#ifdef DHT_ON
  drawProgress(display, 70, "Updating DHT Sensor");
  dht.update(&readyForDHTUpdate);
  ledPulse(LED2, 250);
#endif

#ifdef THINGSPEAK_ON
  drawProgress(display, 80, "Updating thingspeak...");
  thingspeak.getLastChannelItem(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_READ_KEY);
#endif

  drawProgress(display, 100, "Done...");
  ledPulse(LED2, 250);
}

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x,
                  int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
#ifdef WEATHER_ON
  display->setFont(ArialMT_Plain_10);
  String date = me->getTimeProvider()->getDateString();
  display->drawString(64 + x, 5 + y, date);
#endif
  display->setFont(ArialMT_Plain_24);
  String time = me->getTimeProvider()->getTimeStringLong();
  display->drawString(64 + x, 15 + y, time);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

#ifdef POWER_ON
void drawInstantPower(OLEDDisplay *display, OLEDDisplayUiState *state,
                      int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(42 + x, 5 + y, "Instant power");

  display->setFont(ArialMT_Plain_24);
  display->drawString(42 + x, 15 + y, formattedInstantPower);

  display->drawXbm(0 + x, 5 + y, zap_width, zap_height, zap1);
}

void drawEnergyConsumption(OLEDDisplay *display, OLEDDisplayUiState *state,
                           int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0, "Energy Consumption");
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 12, "Y-day: ");
  display->drawString(0 + x, 30, "Today: ");
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(
      128 + x, 12, String(COALESCE_D(powerStats[POWER_P_YESTERDAY])) + " kWh");
  display->drawString(128 + x, 30,
                      String(COALESCE_D(powerStats[POWER_P_TODAY])) + " kWh");
}

void drawEnergyReadings(OLEDDisplay *display, OLEDDisplayUiState *state,
                        int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0, "Meter Readings");
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 12, "T1 : ");
  display->drawString(0 + x, 30, "T2 : ");
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128 + x, 12,
                      String(COALESCE_D(powerStats[POWER_P_T1])) + " kWh");
  display->drawString(128 + x, 30,
                      String(COALESCE_D(powerStats[POWER_P_T2])) + " kWh");
}
#endif

#ifdef WEATHER_ON
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState *state,
                        int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(60 + x, 5 + y, String(COALESCE_D(weatherText[0])));

  display->setFont(ArialMT_Plain_24);
  String temp = String(COALESCE_D(weatherItems[WEATHER_P_TEMP])) + "°C";
  display->drawString(60 + x, 15 + y, temp);

  display->setFont(Meteocons_Plain_42);
  String weatherIcon = String(COALESCE(weatherItems[WEATHER_P_ICON],")")); // wunderground.getTodayIcon();
  int weatherIconWidth = display->getStringWidth(weatherIcon);
  display->drawString(32 + x - weatherIconWidth / 2, 05 + y, weatherIcon);
}

void drawForecast(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 1);
  drawForecastDetails(display, x + 44, y, 2);
  drawForecastDetails(display, x + 88, y, 3);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String day = String(
      COALESCE_D(weatherItems[dayIndex * WEATHER_PARAMS + WEATHER_P_DAY]));

  day.toUpperCase();
  display->drawString(x + 20, y, day);

  display->setFont(Meteocons_Plain_21);
  display->drawString(
      x + 20, y + 12,
      COALESCE(weatherItems[dayIndex * WEATHER_PARAMS + WEATHER_P_ICON], ")"));

  display->setFont(ArialMT_Plain_10);
  display->drawString(
      x + 20, y + 34,
      COALESCE_D(weatherItems[dayIndex * WEATHER_PARAMS + WEATHER_P_TEMP]));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}
#endif

#ifdef DHT_ON
void drawIndoor(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0, "Indoor Sensor");
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 12, "Temp: " + String(dht.formattedTemperature()) + "°C");
  display->drawString(64 + x, 30, "Humidity: " + String(dht.formattedHumidity()) + "%");
}
#endif

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state) {
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, me->getTimeProvider()->getTimeStringShort());

#ifdef POWER_ON
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 54, formattedInstantPower);
#endif

#ifdef WEATHER_ON
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = String(COALESCE_D(weatherItems[WEATHER_P_TEMP])) + "°C";
  display->drawString(128, 54, temp);
  display->drawHorizontalLine(0, 52, 128);
#endif
}

#ifdef DHT_ON
void setReadyForDHTUpdate() {
  Serial.println("Ticker: readyForDHTUpdate");
  readyForDHTUpdate = true;
}
#endif

void setReadyToPublishData() {
  Serial.println("Ticker: readyToPublishData");
  readyToPublishData = true;
}

void publishData() {
  Serial.println("Publishing data ...");
  readyToPublishData = false;
  ledSet(LED2, HIGH);

#ifdef POWER_ON
  power->updateData();
  if (me->mqttPublish(
          "power", power->getDataJson(MQTT_DATA_COLLECTION_PERIOD_SECS), false))
    power->clearKept();
  else
    Serial.println("MQTT publish fail");

#endif

#ifdef DHT_ON
  me->mqttPublish("temperature", dht.getDataJson(), true);
#endif

  Serial.println("Publishing data finished.");
  ledSet(LED2, LOW);
}

const char *formattedInstantPowerW(double watts) {
#define FIP_BUF_LEN 10
  static char formattedInstantPower[FIP_BUF_LEN + 1];
  if (watts < 10)
    snprintf(formattedInstantPower, FIP_BUF_LEN, ("-- W"));
  else if (watts < 1000)
    snprintf(formattedInstantPower, FIP_BUF_LEN, ("%s W"),
             formatDouble41(watts));
  else if (watts < 1000000)
    snprintf(formattedInstantPower, FIP_BUF_LEN, ("%s kW"),
             formatDouble41(watts / 1000));
  else
    snprintf(formattedInstantPower, FIP_BUF_LEN, ("9999.9 kW"));
  return formattedInstantPower;
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

void onMqttEvent(EspNodeBase::MqttEvent event, const char *topic,
                 const char *message, unsigned int len) {
  switch (event) {
  case EspNodeBase::MqttEvent::CONNECT:
    Serial.println("onMqttEvent::CONNECT");
    drawEndlessProgress((char *)F("Connecting to MQTT"), true);
    subscribeToMqttTopics();
    break;
  case EspNodeBase::MqttEvent::DISCONNECT:
    Serial.println("onMqttEvent::DISCONNECT");
    drawEndlessProgress((char *)F("Connecting to MQTT"));
    break;
  case EspNodeBase::MqttEvent::MESSAGE:
    Serial.println("onMqttEvent::MESSAGE");
    if (strcmp(topic, "readings") == 0)
      parseDelimetedString(bufPowerStats, powerStats, POWER_PARAMS, message,
                           len);

    else if (strcmp(topic, "weather") == 0)
      parseDelimetedString(bufWeather, weatherItems,
                           WEATHER_DAYS * WEATHER_PARAMS, message, len);
    else if (strcmp(topic, "weather-t") == 0)
      parseDelimetedString(bufWeatherText, weatherText, WEATHER_DAYS, message,
                           len);
    break;

  }
}

void setupNetworkHandlers() {
  me->registerWifiHandler([](EspNodeBase::WifiEvent e, int counter) {
    if (e == EspNodeBase::WifiEvent::CONNECTING) {
      drawEndlessProgress((char *)F("Connecting to WiFi"));
      ledPulse(LED2, 250);
    } else if (e == EspNodeBase::WifiEvent::FAILURE) {
      drawEndlessProgress((char *)F("WiFi failure..."));
      ledPulse(LED2, 250);
    }
  });
  me->registerOtaHandlers(
      []() {
        display->clear();
        display->setFont(ArialMT_Plain_10);
        display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
        display->drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10,
                            (char *)F("OTA Update"));
        display->display();
      },
      []() {
        display->clear();
        display->setFont(ArialMT_Plain_10);
        display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
        display->drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2,
                            (char *)F("Restart"));
        display->display();
        publishData(); // publish data before restart
      },
      [](unsigned int progress, unsigned int total) {
        int p = progress / (total / 100);
        display->drawProgressBar(4, 32, 120, 8, p);
        display->display();
        ledSet(LED2, (p & 1) ? LOW : HIGH);
      },
      NULL);
  me->addMqttEventHandler(onMqttEvent);
}

bool knockKnock = false;
/*
void doorBellCallback() {
  Serial.println("Door bell, knock-knock!");
  knockKnock = true;
}
*/
void publishDoorBell() {
  Serial.println("Publishing door bell ...");
  if (me->mqttPublish("doorbell", "true", false)) {
    knockKnock = false;
  } else {
    Serial.println("MQTT publish fail");
  }
}

void setupDoorBell() {
  DoorBell::setup();
  DoorBell::setCallback([](){
    Serial.println("Door bell, knock-knock!");
    knockKnock = true;
  });
}

void setup() {
#ifdef POWER_ON
  //setup ISR first to minimize lost pulses during reset/power on
  power->setup();
#endif

  Serial.begin(SERIAL_BAUD_RATE);
  delayMs(50);
  Serial.println("Entering setup.");
  delayMs(50);
  
  me->setup();
  
  setupLed(LED1);
  setupLed(LED2);
  initDisplay();

  setupNetworkHandlers();
  me->startNetworkStack();

  initUi();
  setupRegularActions();
  setupDoorBell();

  Serial.println("Setup finished.");
}

void loop() {
  me->loop();
  yield();
  
#ifdef POWER_ON
  ledSet(LED1, digitalRead(POWER_PULSE_PIN));
  power->loop();
  formattedInstantPower = power->formattedInstantPowerW(true);
#endif

  DoorBell::loop();
  // Let's make sure frame transition is over
  if (ui->getUiState()->frameState == FIXED) {
    if (knockKnock && me->isConnected()) {
      Serial.println("UI is still. Let's publish door bell");
      publishDoorBell();
    }
  }

  // Let's make sure frame transition is over
  if (ui->getUiState()->frameState == FIXED) {
    if (readyToPublishData && me->isConnected()) {
      Serial.println("UI is still. Let's publish data");
      publishData();
    }

#ifdef DHT_ON
    readyForDataUpdate |= readyForDHTUpdate;
#endif

    if (readyForDataUpdate)
      updateData();
  }

  int remainingTimeBudget = ui->update();
  if (remainingTimeBudget > 0) {
    delayMs(remainingTimeBudget);
  }
}
