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
   // Please read http://blog.squix.org/weatherstation-getting-code-adapting-it
   // for setup instructions
 */
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <JsonListener.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>
#include <Wire.h>
#include <WundergroundClient.h>
#include <TimeClient.h>

#include <PubSubClient.h>
#include <ESP8266mDNS.h>  //For OTA
#include <WiFiUdp.h>      //For OTA
#include <ArduinoOTA.h>   //For OTA

#include "res/WeatherStationFonts.h"
#include "res/WeatherStationImages.h"
#include "res/LatonitaIcons.h"

#include "LatonitaUtils.h"

#include "config.h"


#ifdef DHT_ON
  #include "dht11.h"
#endif

#ifdef THINGSPEAK_ON
  #include "ThingspeakClient.h"
#endif

#define ledOn() digitalWrite(LED_PIN,HIGH)
#define ledOff() digitalWrite(LED_PIN,LOW)
#define ledSet(x) digitalWrite(LED_PIN,x)
#define ledPulse(x) ledOn();delayMs(2*x/3);ledOff();delayMs(x/3)
#define setupLed() pinMode(LED_PIN,OUTPUT); ledOff()

#define led2On() digitalWrite(LED_PIN2,HIGH)
#define led2Off() digitalWrite(LED_PIN2,LOW)
#define led2Set(x) digitalWrite(LED_PIN2,x)
#define led2Pulse(x) led2On();delayMs(2*x/3);led2Off();delayMs(x/3)
#define setupLed2() pinMode(LED_PIN,OUTPUT); led2Off()


/***************************
 * Network
 **************************/
String hostName(HOSTNAME_BASE);
WiFiClient wifiClient;
WiFiServer TelnetServer(8266); // Necesary to make Arduino Software autodetect OTA device
PubSubClient mqttClient(wifiClient);

/***************************
 * DHT sensor
 **************************/
#ifdef DHT_ON
  char FormattedTemperature[10];
  char FormattedHumidity[10];
  // Initialize the temperature/ humidity sensor
  dht11 dht;
  #define ALPHA 0.7
  bool dhtInitialized = false;
  float humidity = 0.0;
  float temperature = 0.0;
#endif

/***************************
 * TimeClient
 **************************/
TimeClient timeClient(UTC_OFFSET);
String lastUpdate = "--";

/***************************
 * Wunderground Settings
 **************************/
const boolean IS_METRIC = true;
const String WUNDERGRROUND_API_KEY = "f886e212d6bc0877";
const String WUNDERGRROUND_LANGUAGE = "EN";
const String WUNDERGROUND_COUNTRY = "RU";
const String WUNDERGROUND_CITY = "Saint_Petersburg";
WundergroundClient wunderground(IS_METRIC); // Set to false, if you prefere imperial/inches, Fahrenheit

/***************************
 * ThingSpeak Settings
 **************************/
#ifdef THINGSPEAK_ON
  //Thingspeak Settings
  const String THINGSPEAK_CHANNEL_ID = "67284";
  const String THINGSPEAK_API_READ_KEY = "L2VIW20QVNZJBLAK";
  ThingspeakClient thingspeak;
#endif

/***************************
 * display objects
 **************************/
SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi ui( &display );

/***************************
 * Ticker and flags
 **************************/
#define addRegularAction(x, y) { Ticker* t = new Ticker(); t->attach(x,y);}

bool readyToPublishData = false;
void setReadyToPublishData();

bool readyForWeatherUpdate = false;
void setReadyForWeatherUpdate();

#ifdef DHT_ON
  bool readyForDHTUpdate = false;
  void setReadyForDHTUpdate();
#endif

/***************************
 * forward declarations
 **************************/

void updateDHT();
void updateData(OLEDDisplay *display);

void drawProgress(OLEDDisplay *display, int percentage, String label);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawInstantPower(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
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

// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTime, drawInstantPower, drawCurrentWeather, drawForecast
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


/***************************
 * Power counting
 **************************/
typedef struct {
  unsigned long count = 0;
  unsigned long timeMicros = 0;
  unsigned long lastTimeMicros = 0;
} PulseParamISR;
volatile PulseParamISR powerPulse;

typedef struct {
  unsigned int pulsesKept = 0;
  unsigned int secondsKept = 0;

  unsigned int instantWatts = 0;
  unsigned int consumedkWh = 0;

  unsigned int consumedTodaykWh = 0;
  unsigned int consumedYesterdaykWh = 0;
  unsigned long lastSeconds = 0;
} PowerDisplay;
PowerDisplay power;

char formattedInstantPower[10];
static char tempBuffer32[32];


// The interrupt routine - runs each time a falling edge of a pulse is detected
#define PULSE_DEBOUNCE 100000L
void onPowerPulseISR() {
  if (digitalRead(PULSE_PIN) == LOW) {
    if(micros() >= powerPulse.timeMicros + PULSE_DEBOUNCE ) {
      powerPulse.count++;
      powerPulse.lastTimeMicros = powerPulse.timeMicros;      //used to measure time between pulses.
      powerPulse.timeMicros = micros();
    }
  }
}

void setupPowerPulsesCounting() {
    pinMode(PULSE_PIN, INPUT);
    attachInterrupt(PULSE_PIN, onPowerPulseISR, FALLING);
}

void setupRegularActions() {
  addRegularAction(MQTT_DATA_COLLECTION_PERIOD_SECS, setReadyToPublishData);
  addRegularAction(FORECAST_UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);
  #ifdef DHT_ON
    addRegularAction(DHT_UPDATE_INTERVAL_SECS, setReadyForDHTUpdate);
  #endif
}

void printVersion() {
  const char compileDate[] = __DATE__ " " __TIME__;
  Serial.println();
  Serial.println("===============================================================");
  Serial.println(": MQTT Pulse counter and MQTT display with OTA                :");
  Serial.println(": (c) Anton Viktorov, latonita@yandex.ru, github.com/latonita :");
  Serial.println("===============================================================");
  Serial.printf("Compile date: %s\r\n", compileDate);
  Serial.printf("Module id: %s\r\n", hostName.c_str());
  Serial.printf("Wireless %s\r\n", WIFI_SSID);
  Serial.printf("MQTT server %s\r\n", MQTT_SERVER);
  Serial.printf("MQTT topic %s\r\n", MQTT_BASE_TOPIC);
  Serial.println("===============================================================");
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
  Serial.println("UI Init...");
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

// void setupLed() {
//   pinMode(LED_PIN, OUTPUT);
//   digitalWrite(LED_PIN, LOW);
// }
//
// static char ledState = LOW;
// void changeLed() {
//   ledState ^= HIGH;
//   digitalWrite(LED_PIN, ledState);
// }
//
// void setLed(char state) {
//   ledState = state;
//   digitalWrite(LED_PIN, ledState);
// }

void mqttReconnect();
void setupMqtt() {
    Serial.println("Configuring MQTT server...");
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    Serial.printf("MQTT server IP: %s\r\n", MQTT_SERVER);
    Serial.println("MQTT configured!");
    mqttReconnect();
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
    digitalWrite(LED_PIN, (p&1) ? LOW : HIGH);
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
  Serial.println(" Done.");
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

void setupWifi() {
  WiFi.hostname(hostName);
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  Serial.print("Connecting to WiFi..");
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
      led2Pulse(250);
      yield();
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
  setupLed();
  setupLed2();
  led2Pulse(100);led2Pulse(100);led2Pulse(100);led2Pulse(100);

  hostName += String(ESP.getChipId(), HEX);
  Serial.begin(SERIAL_BAUD_RATE);
  delayMs(50);
  printVersion();
  Serial.println("Entering setup.");
  initDisplay();
  setupWifi();
  setupOta();
  setupMqtt();
  initUi();
  setupPowerPulsesCounting();
  setupRegularActions();
  Serial.println("Setup finished.");
}

void formatInstantPower(double watts) {
  if (watts < 10) {
    sprintf(formattedInstantPower, "-- W");
  } else if (watts < 1000) {
    dtostrf(watts, 4, 0, tempBuffer32);
    sprintf(formattedInstantPower, "%s W", tempBuffer32);
  } else {
    dtostrf(watts/1000, 4, 1, tempBuffer32);
    sprintf(formattedInstantPower, "%s kW", tempBuffer32);
  }
}

void powerCalculationLoop() {
  // today/y-day
  unsigned long secondsOfDay = timeClient.getCurrentEpochWithUtcOffset()  % 86400L;
  if (power.lastSeconds > secondsOfDay) {
    power.lastSeconds = secondsOfDay;
    power.consumedYesterdaykWh = power.consumedTodaykWh;
    power.consumedTodaykWh = 0;
  }

  // calc power consumption
  // 640 p per 1kWh => 1p = 1.5625 Wh
  //                   1Wh = 0.640p
  #define ppwh (1000/640)
  #define whpp (640/1000)

//  float wattsSpent = 1.5625 * (power.pulsesKept + powerPulse.count);


  //    emontx.power = int((3600000000.0 / (pulseTime - lastTime)) / ppwh); //Calculate power
  //int instantPower = 1000 * 3600 * p / (s * 640);
  //  unsigned int instantWatts = 0;

//pulses per watt hour

  static char firstRun = 1;
  static double watts = 0;
  static unsigned long m = 0;
  if (millis() > m + 15*1000) {
    m = millis();

    // first time. rollover is not a problem
    if (powerPulse.lastTimeMicros == 0) {
      watts = 0;
    } else {
      watts = int((3600000000.0 / (micros() - powerPulse.lastTimeMicros)) / ppwh); //Calculate power
    }
    formatInstantPower(watts);

    //debug
    Serial.printf("-------\r\n");
    Serial.printf("insta watt: %s\r\n",formattedInstantPower);
    Serial.printf("pulses: %d\r\n", powerPulse.count);
    Serial.printf("pulses kept: %d\r\n", power.pulsesKept);
    unsigned int wh = (powerPulse.count + power.pulsesKept) * 640 / 1000;
    Serial.printf("consumed energy: %d Wh\r\n", wh);
    Serial.println(getUpTime());
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
    Serial.println("Data update...");
    drawProgress(display, 10, "Updating time...");
    timeClient.updateTime();
    led2Pulse(250);
    drawProgress(display, 30, "Updating conditions...");
    wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    led2Pulse(250);
    drawProgress(display, 50, "Updating forecasts...");
    wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    led2Pulse(250);

#ifdef DHT_ON
    drawProgress(display, 70, "Updating DHT Sensor");
    updateDHT();
    led2Pulse(250);
#endif

#ifdef THINGSPEAK_ON
    drawProgress(display, 80, "Updating thingspeak...");
    thingspeak.getLastChannelItem(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_READ_KEY);
#endif

    lastUpdate = timeClient.getFormattedTime();
    readyForWeatherUpdate = false;
    drawProgress(display, 100, "Done...");
    led2Pulse(250);
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

void drawInstantPower(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(42 + x, 5 + y, "Instant power");

    display->setFont(ArialMT_Plain_24);
    display->drawString(42 + x, 15 + y, formattedInstantPower);
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

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
    display->setColor(WHITE);
    display->setFont(ArialMT_Plain_10);
    String time = timeClient.getFormattedTime().substring(0, 5);

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 54, time);

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(64, 54, formattedInstantPower);

    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    String temp = wunderground.getCurrentTemp() + "°C";
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


void setReadyForWeatherUpdate() {
  Serial.println("Ticker: readyForWeatherUpdate");
  readyForWeatherUpdate = true;
}

void setReadyForDHTUpdate() {
  Serial.println("Ticker: readyForDHTUpdate");
  readyForDHTUpdate = true;
}

void setReadyToPublishData() {
  Serial.println("Ticker: readyToPublishData");
  readyToPublishData = true;
}

#ifdef DHT_ON
void updateDHT() {
  readyForDHTUpdate = false;
  if (DHTLIB_OK == dht.read(DHT_PIN)) {
    Serial.print("DHT read: "); Serial.print(dht.temperature); Serial.print("C "); Serial.print(dht.humidity); Serial.println("\%");
    humidity = !dhtInitialized ? dht.humidity : ALPHA * humidity + (1 - ALPHA) * (float)dht.humidity;
    temperature = !dhtInitialized ? dht.temperature : ALPHA * temperature + (1 - ALPHA) * (float)dht.temperature;
    dhtInitialized = true;
  } else {
    Serial.println("DHT reading failure.");
  }
}
#endif

const static String mqttBaseTopic = MQTT_BASE_TOPIC;

#define MQTT_ONLINE_TOPIC MQTT_BASE_TOPIC "$online"
#define MQTT_MSG_ONLINE "online"
#define MQTT_MSG_OFFLINE "offline"

//#define MQTT_BASE_TOPIC "world/p14/sensors/entrance/"
void mqttReconnect() {
//  backToSystemPage();
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    led2Pulse(250);
//    updateDisplay();
    yield();

    // boolean connect (clientID, willTopic, willQoS, willRetain, willMessage)
    if (mqttClient.connect(hostName.c_str(), MQTT_ONLINE_TOPIC, 0, 1, MQTT_MSG_OFFLINE)) {
      Serial.println("connected");
      mqttClient.publish(MQTT_ONLINE_TOPIC, MQTT_MSG_ONLINE, true);
      mqttClient.publish((mqttBaseTopic + "$id").c_str(), hostName.c_str(), true);
      mqttClient.publish((mqttBaseTopic + "$ip").c_str(), WiFi.localIP().toString().c_str(), true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in few seconds");
//      updateDisplay();
      for (int i = 0; i < 5; i++) {
        led2Pulse(250);
      }
    }
  }
}



//
// about mqtt naming
//
// myhome/sensors/entrance/$online = online/offline
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

//#define MQTT_BASE_TOPIC "world/p14/sensors/entrance/"

void publishData() {
  Serial.println("Publishing data ...");
  readyToPublishData = false;

  cli();
  power.pulsesKept += powerPulse.count;
  power.secondsKept += MQTT_DATA_COLLECTION_PERIOD_SECS;
  powerPulse.count = 0;
  sei();
  sprintf(tempBuffer32,"%d;%d", power.pulsesKept, power.secondsKept);
  if (mqttClient.publish((mqttBaseTopic + "power/pulses_raw").c_str(), itoa(humidity, tempBuffer32, 10), false)) {
    power.pulsesKept = power.secondsKept = 0;
  } else {
    Serial.println("MQTT publish fail");
  }

  mqttClient.publish((mqttBaseTopic + "temperature").c_str(), itoa(temperature, tempBuffer32, 10), true);
  mqttClient.publish((mqttBaseTopic + "humidity").c_str(), itoa(humidity, tempBuffer32, 10), true);

  mqttClient.publish((mqttBaseTopic + "$stats/uptime").c_str(), itoa(millis()/1000, tempBuffer32, 10), false);
  mqttClient.publish((mqttBaseTopic + "$stats/signal").c_str(), itoa(getWifiQuality(), tempBuffer32, 10), false);
  Serial.println("Publishing data finished.");
}

void loop() {
  ledSet(digitalRead(PULSE_PIN));
  led2On();

  ArduinoOTA.handle();
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  yield();
  powerCalculationLoop();

  // Let's make sure frame transition is over
  if (ui.getUiState()->frameState == FIXED) {
    if (readyToPublishData) {
      publishData();
    }
    if (readyForWeatherUpdate) {
      updateData(&display);
    }
    #ifdef DHT_ON
    if (readyForDHTUpdate) {
      updateDHT();
    }
    #endif
  }

  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
      delayMs(remainingTimeBudget);
  }
}
