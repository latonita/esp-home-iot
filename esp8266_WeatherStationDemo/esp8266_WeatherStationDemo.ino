/*
   2017. Anton Viktorov. Preface.
   This work is based on Daniel Eichhorn's Weather Station Demo sketch.
   I used to have quite similar application, but due to unknown reasons all glitchy
   chineese esp8266 modules stopped accepting my sketch, rebooting even before
   entering 'setup()' function. I have spent 2 days not able to solve an issue and
   was ready to break my monitor with an empty wisky bottle. And I just tried this
   sketch and it started working, oceans bless my monitor... So I started updating
   this sketch trying to get same functionality (+extra) that I had before.

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

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

   See more at http://blog.squix.ch
 */
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <JsonListener.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "Wire.h"
#include "WundergroundClient.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "LatonitaIcons.h"
#include "TimeClient.h"

#include <PubSubClient.h>
#include <ESP8266mDNS.h>  //For OTA
#include <WiFiUdp.h>      //For OTA
#include <ArduinoOTA.h>   //For OTA

/***************************
 * Features on/off
 **************************/
#define DHT_ON
//#define THINGSPEAK_ON

#ifdef DHT_ON
#include "dht11.h"
#endif

#ifdef THINGSPEAK_ON
#include "ThingspeakClient.h"
#endif

const char compileDate[] = __DATE__ " " __TIME__;
/***************************
 * Begin Settings
 **************************/
// Please read http://blog.squix.org/weatherstation-getting-code-adapting-it
// for setup instructions

// ALL PINS HERE
#define PULSE_PIN 14    // kwh pulses
#define ANOTHER_PIN 13  // one more signal pin available, will be shorted to ground by external device
#define LED_PIN 15
#define SDA_PIN 5
#define SDC_PIN 4
#define DHT_PIN 12

#define SERIAL_BAUD_RATE 74880

// Network
#define HOSTNAME_BASE "espnode-"
String hostName(HOSTNAME_BASE);

// WIFI
const char* WIFI_SSID = "LAWIRELESS";
const char* WIFI_PWD = "MegaPass!";

WiFiClient wifiClient;
WiFiServer TelnetServer(8266);        // Necesary to make Arduino Software autodetect OTA device
PubSubClient mqttClient(wifiClient);

// OTA configuration
#define OTA_PASSWORD "Secret!"

// MQTT
#define MQTT_DATA_COLLECTION_PERIOD_SECS 5 * 60 // time to collect data before posting to mqtt
#define MQTT_SERVER "192.168.3.3"
#define MQTT_BASE_TOPIC "p14/sensors/entrance/"

// myhome/sensors/entrance/$online
// myhome/sensors/entrance/$id
// myhome/sensors/entrance/$ip
// myhome/sensors/entrance/$stats/uptime = seconds
// myhome/sensors/entrance/$stats/signal = signal strength
// myhome/sensors/entrance/$fw = firmware string

// myhome/sensors/entrance/power/pulses_raw  = pulses;period
// myhome/sensors/entrance/power/instant  = instant watts
// myhome/sensors/entrance/power/usage = wh since last time
// myhome/sensors/entrance/temperature  = temp
// myhome/sensors/entrance/humidity  = temp
// myhome/sensors/entrance/doorbell = dtm;photo

// myhome/sensors/bathroom/water/pulses_raw = pulses_cold;pulses_hot;period

// data to show:
// myhome/presentation/power ///// ????
// myhome/presentation/alert/

//to read: http://tinkerman.cat/mqtt-topic-naming-convention/
//https://harizanov.com/2014/09/mqtt-topic-tree-structure-improvements/
//https://forum.mysensors.org/topic/4341/mqtt-messages-and-sensor-tree/4
//https://hackaday.io/project/7342-mqopen/log/33302-mqtt-topic-structure-redesign


// Setup
#define FORECAST_UPDATE_INTERVAL_SECS 30 * 60 // Update weather forecast every 30 minutes


#ifdef DHT_ON
char FormattedTemperature[10];
char FormattedHumidity[10];
#endif

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
// TimeClient settings
const float UTC_OFFSET = 3;

// Wunderground Settings
const boolean IS_METRIC = true;
const String WUNDERGRROUND_API_KEY = "f886e212d6bc0877";
const String WUNDERGRROUND_LANGUAGE = "EN";
const String WUNDERGROUND_COUNTRY = "RU";
const String WUNDERGROUND_CITY = "Saint_Petersburg";

#ifdef THINGSPEAK_ON
//Thingspeak Settings
const String THINGSPEAK_CHANNEL_ID = "67284";
const String THINGSPEAK_API_READ_KEY = "L2VIW20QVNZJBLAK";
#endif

// Initialize the oled display for address 0x3c
// sda-pin=14 and sdc-pin=12
SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi ui( &display );

/***************************
 * End Settings
 **************************/

TimeClient timeClient(UTC_OFFSET);

// Set to false, if you prefere imperial/inches, Fahrenheit
WundergroundClient wunderground(IS_METRIC);

#ifdef DHT_ON
// Initialize the temperature/ humidity sensor
dht11 dht;
#define ALPHA 0.7
bool dhtInitialized = false;
float humidity = 0.0;
float temperature = 0.0;
#endif

//power counting
typedef struct {
    int power, pulse;
} PayloadTX;

volatile PayloadTX emontx;

// Pulse counting settings
long pulseCount = 0;                                                    // Number of pulses, used to measure energy.
unsigned long pulseTime,lastTime;                                       // Used to measure power.
double power, elapsedWh;                                                // power and energy
#define ppwh (100/64)                                                           // 1000 pulses/kwh = 1 pulse per wh - Number of pulses per wh - found or set on the meter.

char formattedInstantPower[10] = "--";
float instantPower = 0;

#ifdef THINGSPEAK_ON
ThingspeakClient thingspeak;
#endif

// flag changed in the ticker function every xxx minutes
bool readyToPublishData = false;
bool readyForWeatherUpdate = false;
#ifdef DHT_ON
// flag changed in the ticker function every 1 minute
bool readyForDHTUpdate = false;
#define DHT_UPDATE_INTERVAL_SECS 60
#endif

String lastUpdate = "--";

Ticker ticker;

//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawKWH(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
#ifdef DHT_ON
void drawIndoor(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
#endif
#ifdef THINGSPEAK_ON
void drawThingspeak(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
#endif
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyToPublishData();
void setReadyForWeatherUpdate();
void setReadyForWeatherUpdate();
void setReadyForDHTUpdate();

// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTime, drawKWH, drawCurrentWeather, drawForecast
#ifdef DHT_ON
                           , drawIndoor
#endif
#ifdef THINGSPEAK_ON
                           , drawThingspeak
#endif
};
int numberOfFrames = (sizeof(frames) / sizeof(FrameCallback));

OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;


// The interrupt routine - runs each time a falling edge of a pulse is detected
void onPowerPulse() {
    lastTime = pulseTime;      //used to measure time between pulses.
    pulseTime = micros();
    pulseCount++;                                                    //pulseCounter
    emontx.power = int((3600000000.0 / (pulseTime - lastTime)) / ppwh); //Calculate power
}

void setupPowerPulsesCounting() {
    pinMode(PULSE_PIN, INPUT);
    attachInterrupt(PULSE_PIN, onPowerPulse, RISING);
}

void setupRegularActions() {
  ticker.attach(MQTT_DATA_COLLECTION_PERIOD_SECS, setReadyToPublishData);
  ticker.attach(FORECAST_UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);
#ifdef DHT_ON
  ticker.attach(DHT_UPDATE_INTERVAL_SECS, setReadyForDHTUpdate);
#endif
}

void printVersion() {
  Serial.println("MQTT Pulse counter and MQTT display with OTA (C) Anton Viktorov, latonita@yandex.ru, github.com/latonita");
  Serial.print("Compile Date: "); Serial.println(compileDate);
  Serial.print("Module Id: "); Serial.println(hostName);
}
void initDisplay() {
  // initialize dispaly
  display.init();
  display.clear();
  display.display();
  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(180);
}

void initUi() {
  ui.setTargetFPS(30);
  //ui.setActiveSymbol(activeSymbole);
  //ui.setInactiveSymbol(inactiveSymbole);
  ui.setActiveSymbol(emptySymbol);    // Hack until disableIndicator works
  ui.setInactiveSymbol(emptySymbol);  // - Set an empty symbol
  ui.disableIndicator();

  // You can change this to TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);
  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);
  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, numberOfFrames);
  ui.setOverlays(overlays, numberOfOverlays);
  // Inital UI takes care of initalising the display too.
  ui.init();
  updateData(&display);
}

void setupOta() {
  Serial.print("Configuring OTA device...");

  TelnetServer.begin();   //Necesary to make Arduino Software autodetect OTA device

  ArduinoOTA.onStart([]() {
    Serial.println("OTA starting...");
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 10, "OTA Update");
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("OTA update finished!");
    Serial.println("Rebooting...");
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "Restart");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int p = progress / (total / 100);
    Serial.printf("OTA in progress: %u%%\r\n", p );
//    digitalWrite(WIFI_LED_PIN,p%2>0?LOW:HIGH);
    display.drawProgressBar(4, 32, 120, 8, p );
    display.display();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
  ArduinoOTA.setHostname(hostName.c_str());
  ArduinoOTA.begin();
}
void setupWifi() {
  WiFi.hostname(hostName);
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  Serial.print("Connecting to WiFi..");
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      display.clear();
      display.drawString(64, 10, "Connecting to WiFi");
      display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
      display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
      display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
      display.display();

      counter++;
      //todo: check if we cant connect for too long. options - reboot, buzz, setup own wifi?
  }
  Serial.println();
  Serial.print("Got IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
    hostName += String(ESP.getChipId(), HEX);
    Serial.begin(SERIAL_BAUD_RATE);
    printVersion();
    Serial.println("Entering setup.");
    initDisplay();

    setupWifi();
    setupOta();

    initUi();

    setupPowerPulsesCounting();
    setupRegularActions();

    Serial.println("Setup finished.");
}


void loop() {
    ArduinoOTA.handle();

    //emontx.pulse = pulseCount;
    emontx.pulse += pulseCount;
    pulseCount = 0;

    if (readyToPublishData && ui.getUiState()->frameState == FIXED) {
        publishData();
    }

    if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
        updateData(&display);
    }

#ifdef DHT_ON
    if (readyForDHTUpdate && ui.getUiState()->frameState == FIXED) {
        updateDHT();
    }
#endif

    int remainingTimeBudget = ui.update();

    if (remainingTimeBudget > 0) {
        // You can do some work here
        // Don't do stuff if you are below your
        // time budget.
        delay(remainingTimeBudget);
    }

    static unsigned long m = 0;
    if (m + 1000 < millis()) {
      m = millis();


      emontx.power = instantPower = int((3600000000.0 / (micros() - lastTime)) / ppwh); //Calculate power
      Serial.print(emontx.power);
      Serial.print("W ");
      Serial.println(emontx.pulse);
  //instantPower = emontx.power;
      dtostrf(instantPower, 4, 1, formattedInstantPower);
    }
}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
    display->clear();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64, 10, label);
    display->drawProgressBar(2, 28, 124, 10, percentage);
    display->display();
}

void updateData(OLEDDisplay *display) {
    drawProgress(display, 10, "Updating time...");
    timeClient.updateTime();
    drawProgress(display, 30, "Updating conditions...");
    wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    drawProgress(display, 50, "Updating forecasts...");
    wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);

#ifdef DHT_ON
    drawProgress(display, 70, "Updating DHT Sensor");
    updateDHT();
    delay(250);
#endif

#ifdef THINGSPEAK_ON
    drawProgress(display, 80, "Updating thingspeak...");
    thingspeak.getLastChannelItem(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_READ_KEY);
#endif

    lastUpdate = timeClient.getFormattedTime();
    readyForWeatherUpdate = false;
    drawProgress(display, 100, "Done...");
    delay(1000);
}

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    String date = wunderground.getDate();
    int textWidth = display->getStringWidth(date);
    display->drawString(64 + x, 5 + y, date);
    display->setFont(ArialMT_Plain_24);
    String time = timeClient.getFormattedTime();
    textWidth = display->getStringWidth(time);
    display->drawString(64 + x, 15 + y, time);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawKWH(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(42 + x, 5 + y, "Instant power");

    String temp = String(formattedInstantPower) + " kW";
    display->setFont(ArialMT_Plain_24);
    display->drawString(42 + x, 15 + y, temp);
    //int tempWidth = display->getStringWidth(temp);

    display->drawXbm(0 + x, 5 + y, zap_width, zap_height, zap1);
//  display->drawXbm(86, 5 + y, zap_width, zap_height, zap2);
// display->setFont(Meteocons_Plain_42);
// String weatherIcon = wunderground.getTodayIcon();
// int weatherIconWidth = display->getStringWidth(weatherIcon);
// display->drawString(32 + x - weatherIconWidth / 2, 05 + y, weatherIcon);
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(60 + x, 5 + y, wunderground.getWeatherText());

    display->setFont(ArialMT_Plain_24);
    String temp = wunderground.getCurrentTemp() + "°C";
    display->drawString(60 + x, 15 + y, temp);
    int tempWidth = display->getStringWidth(temp);

    display->setFont(Meteocons_Plain_42);
    String weatherIcon = wunderground.getTodayIcon();
    int weatherIconWidth = display->getStringWidth(weatherIcon);
    display->drawString(32 + x - weatherIconWidth / 2, 05 + y, weatherIcon);
}


void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    drawForecastDetails(display, x, y, 0);
    drawForecastDetails(display, x + 44, y, 2);
    drawForecastDetails(display, x + 88, y, 4);
}

#ifdef DHT_ON
void drawIndoor(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 0, "Indoor Sensor");
    display->setFont(ArialMT_Plain_16);
    dtostrf(temperature,4, 1, FormattedTemperature);
    display->drawString(64 + x, 12, "Temp: " + String(FormattedTemperature) + (IS_METRIC ? "°C" : "°F"));
    dtostrf(humidity,4, 1, FormattedHumidity);
    display->drawString(64 + x, 30, "Humidity: " + String(FormattedHumidity) + "%");
}
#endif

#ifdef THINGSPEAK_ON
void drawThingspeak(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 0 + y, "Outdoor");
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, 10 + y, thingspeak.getFieldValue(0) + "°C");
    display->drawString(64 + x, 30 + y, thingspeak.getFieldValue(1) + "%");
}
#endif

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
    day.toUpperCase();
    display->drawString(x + 20, y, day);

    display->setFont(Meteocons_Plain_21);
    display->drawString(x + 20, y + 12, wunderground.getForecastIcon(dayIndex));

    display->setFont(ArialMT_Plain_10);
    display->drawString(x + 20, y + 34, wunderground.getForecastLowTemp(dayIndex) + "|" + wunderground.getForecastHighTemp(dayIndex));
    display->setTextAlignment(TEXT_ALIGN_LEFT);
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
    int32_t dbm = WiFi.RSSI();
    if(dbm <= -100) {
        return 0;
    } else if(dbm >= -50) {
        return 100;
    } else {
        return 2 * (dbm + 100);
    }
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
    display->setColor(WHITE);
    display->setFont(ArialMT_Plain_10);
    String time = timeClient.getFormattedTime().substring(0, 5);

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 54, time);

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    String temp = String(formattedInstantPower) + "kW";
    display->drawString(64, 54, temp);

    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    temp = wunderground.getCurrentTemp() + "°C";
    display->drawString(128, 54, temp);

    display->drawHorizontalLine(0, 52, 128);
/*
   display->setColor(WHITE);
   display->setFont(ArialMT_Plain_10);
   display->setTextAlignment(TEXT_ALIGN_LEFT);
   display->drawString(0, 54, String(state->currentFrame + 1) + "/" + String(numberOfFrames));

   String time = timeClient.getFormattedTime().substring(0, 5);
   display->setTextAlignment(TEXT_ALIGN_CENTER);
   display->drawString(38, 54, time);

   display->setTextAlignment(TEXT_ALIGN_CENTER);
   String temp = wunderground.getCurrentTemp() + "°C";
   display->drawString(90, 54, temp);

   int8_t quality = getWifiQuality();
   for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        display->setPixel(120 + 2 * i, 63 - j);
      }
    }
   }


   display->setTextAlignment(TEXT_ALIGN_CENTER);
   display->setFont(Meteocons_Plain_10);
   String weatherIcon = wunderground.getTodayIcon();
   int weatherIconWidth = display->getStringWidth(weatherIcon);
   display->drawString(64, 55, weatherIcon);

   display->drawHorizontalLine(0, 52, 128);
 */
}

void setReadyToPublishData() {
  Serial.println("Setting readyForUpdate to true");
  readyToPublishData = true;
}

void setReadyForWeatherUpdate() {
    Serial.println("Setting readyForUpdate to true");
    readyForWeatherUpdate = true;
}

void setReadyForDHTUpdate() {
    Serial.println("Setting readyForDHTUpdate to true");
    readyForDHTUpdate = true;
}

#ifdef DHT_ON
void updateDHT() {
    if (DHTLIB_OK == dht.read(DHT_PIN)) {
        Serial.print("DHT read: "); Serial.print(dht.temperature); Serial.print("C "); Serial.print(dht.humidity); Serial.println("\%");
        humidity = !dhtInitialized ? dht.humidity : ALPHA * humidity + (1 - ALPHA) * (float)dht.humidity;
        temperature = !dhtInitialized ? dht.temperature : ALPHA * temperature + (1 - ALPHA) * (float)dht.temperature;
        readyForDHTUpdate = false;
        dhtInitialized = true;
    }
}
#endif

void publishData() {
  readyToPublishData = false;


  // MQTT Conn check

  //mqttClient.publish((mqttBaseTopics + TOPIC_TEMPERATURE).c_str(), String(temp).c_str(), true);
  // setup payload
  // send

}
