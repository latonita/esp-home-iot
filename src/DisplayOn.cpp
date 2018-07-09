#include "DisplayOn.h"
#include "TemperatureSensor.hpp"
#include "PulseMeters.hpp"

SSD1306Wire * DisplayOn::display = NULL;
OLEDDisplayUi * DisplayOn::ui = NULL;

FrameCallback DisplayOn::frames[] = {DisplayOn::drawDateTime
#ifdef POWER_ON
                        , DisplayOn::drawInstantPower
                        , DisplayOn::drawEnergyConsumption
                        , DisplayOn::drawEnergyReadings
#endif
#ifdef WEATHER_ON
                        , DisplayOn::drawCurrentWeather
                        , DisplayOn::drawForecast
#endif
#ifdef DHT_ON
//                        , DisplayOn::drawIndoor
#endif
};
const int DisplayOn::numberOfFrames = (sizeof(DisplayOn::frames) / sizeof(FrameCallback));
OverlayCallback DisplayOn::overlays[] = {DisplayOn::drawHeaderOverlay};

#ifdef WEATHER_ON
  char * DisplayOn::bufWeather = new char[BUF_MAX]; // NULL;
  char * DisplayOn::weatherItems[WEATHER_DAYS * WEATHER_PARAMS] = {
      NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL};  // 4 days x 4 params
  char *DisplayOn::bufWeatherText = new char[BUF_MAX]; // NULL;
  char *DisplayOn::weatherText[WEATHER_DAYS] = {NULL, NULL, NULL, NULL};
#endif

#ifdef POWER_ON
  char *DisplayOn::bufPowerStats = new char[BUF_MAX]; // NULL;
  char *DisplayOn::powerStats[POWER_PARAMS] = {NULL, NULL, NULL, NULL};
  const char *DisplayOn::formattedInstantPower = "";
#endif

void DisplayOn::initDisplayAndUI() {
  Serial.println(F("OLED hardware init..."));
  display = new SSD1306Wire(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
  display->init();
  display->clear();
  display->display();
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setContrast(180);

  Serial.println(F("OLED UI Init..."));
  ui = new OLEDDisplayUi(display);
  ui->setTargetFPS(30);
  // ui->setActiveSymbol(activeSymbole);
  // ui->setInactiveSymbol(inactiveSymbole);
  ui->setActiveSymbol(emptySymbol);   // Hack until disableIndicator works
  ui->setInactiveSymbol(emptySymbol); // - Set an empty symbol
  ui->disableIndicator();
  ui->setIndicatorPosition(BOTTOM);
  ui->setIndicatorDirection(LEFT_RIGHT);
  ui->setFrameAnimation(SLIDE_LEFT);
  ui->setFrames(frames, numberOfFrames);
  ui->setOverlays(overlays, numberOfOverlays);
  ui->init();
}

bool DisplayOn::isFixed() {
    return (ui->getUiState()->frameState == FIXED);
}

void DisplayOn::drawEndlessProgress(const char *msg, bool finished) {
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

void DisplayOn::drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void DisplayOn::drawDateTime(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x,
                  int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
#ifdef WEATHER_ON
  display->setFont(ArialMT_Plain_10);
  String date = EspNodeBase::me()->getTimeProvider()->getDateString();
  display->drawString(64 + x, 5 + y, date);
#endif
  display->setFont(ArialMT_Plain_24);
  String time = EspNodeBase::me()->getTimeProvider()->getTimeStringLong();
  display->drawString(64 + x, 15 + y, time);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

#ifdef POWER_ON
void DisplayOn::drawInstantPower(OLEDDisplay *display, OLEDDisplayUiState *state,
                      int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(42 + x, 5 + y, "Instant power");

  display->setFont(ArialMT_Plain_24);
  display->drawString(42 + x, 15 + y, formattedInstantPower);

  display->drawXbm(0 + x, 5 + y, zap_width, zap_height, zap1);
}

void DisplayOn::drawEnergyConsumption(OLEDDisplay *display, OLEDDisplayUiState *state,
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

void DisplayOn::drawEnergyReadings(OLEDDisplay *display, OLEDDisplayUiState *state,
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
void DisplayOn::drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState *state,
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

void DisplayOn::drawForecast(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 1);
  drawForecastDetails(display, x + 44, y, 2);
  drawForecastDetails(display, x + 88, y, 3);
}

void DisplayOn::drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
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
extern TemperatureSensor dht;

void DisplayOn::drawIndoor(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0, "Indoor Sensor");
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 12, "Temp: " + String(dht.formattedTemperature()) + "°C");
  display->drawString(64 + x, 30, "Humidity: " + String(dht.formattedHumidity()) + "%");
}
#endif

void DisplayOn::drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state) {
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, EspNodeBase::me()->getTimeProvider()->getTimeStringShort());

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

