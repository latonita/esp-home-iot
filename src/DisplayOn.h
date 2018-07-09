#ifndef DISPLAY_ON_H
#define DISPLAY_ON_H

#include "Config.h"
#include "Utils.h"
#include "EspNodeBase.hpp"

#include <OLEDDisplay.h>
#include <OLEDDisplayUi.h>
#include <SSD1306Wire.h>
#include <Wire.h>

#include "res/LatonitaIcons.h"
#include "res/WeatherStationFonts.h"
#include "res/WeatherStationImages.h"

#define BUF_MAX 255

struct DisplayOn {
    static SSD1306Wire *display;
    static OLEDDisplayUi *ui;

    #ifdef WEATHER_ON
        #define WEATHER_DAYS 4
        #define WEATHER_P_DAY 0
        #define WEATHER_P_TEMP 1
        #define WEATHER_P_ICON 2
        #define WEATHER_PARAMS 3
        static char *bufWeather;
        static char *weatherItems[WEATHER_DAYS * WEATHER_PARAMS];
        static char *bufWeatherText;
        static char *weatherText[WEATHER_DAYS];
    #endif

    #ifdef POWER_ON
        #define POWER_P_T1 0
        #define POWER_P_T2 1
        #define POWER_P_YESTERDAY 2
        #define POWER_P_TODAY 3
        #define POWER_PARAMS 4
        static char *bufPowerStats;
        static char *powerStats[POWER_PARAMS];
        static const char *formattedInstantPower;
    #endif

static void initDisplayAndUI();

static bool isFixed();

static void drawProgress(OLEDDisplay *display, int percentage, String label);
static void drawEndlessProgress(const char *msg, bool finished = false);
static void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x,int16_t y);
static void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state);

#ifdef POWER_ON
static void drawInstantPower(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
static void drawEnergyConsumption(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
static void drawEnergyReadings(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
#endif
#ifdef WEATHER_ON
static void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
static void drawForecast(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
static void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
#endif
#ifdef DHT_ON
static void drawIndoor(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
#endif

static FrameCallback frames[];
static const int numberOfFrames;
static OverlayCallback overlays[];
static const int numberOfOverlays = 1;
};

#endif