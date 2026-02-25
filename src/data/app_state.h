#pragma once

#include <Arduino.h>

// ============================================================
// Application state type definitions
// ============================================================
// This header owns every type and constant that describes
// application state.  Actual runtime globals are still
// declared in main.cpp for now; the full collapse into a
// single AppState struct is the Phase 4 refactor.
// ============================================================

// ----------------------------------------------------------
// Screen / navigation
// ----------------------------------------------------------

enum ScreenState {
    CLOCK, SETTINGS, WIFICONFIG, KEYBOARD,
    SSID_INPUT, WEATHERCONFIG, REGIONALCONFIG,
    GRAPHICSCONFIG, FIRMWARE_SETTINGS, COUNTRYSELECT,
    CITYSELECT, LOCATIONCONFIRM, CUSTOMCITYINPUT,
    CUSTOMCOUNTRYINPUT, COUNTRYLOOKUPCONFIRM,
    CITYLOOKUPCONFIRM, COORDSINPUT
};

// ----------------------------------------------------------
// Weather
// ----------------------------------------------------------

struct ForecastData {
    int   code;
    float tempMax;
    float tempMin;
};

// ----------------------------------------------------------
// Recent-cities history
// ----------------------------------------------------------

constexpr int MAX_RECENT_CITIES = 10;

struct RecentCity {
    String city;
    String country;
    String timezone;
    int    gmtOffset;
    int    dstOffset;
};
