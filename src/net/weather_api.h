#pragma once

#include <Arduino.h>

// Returns a short description string for an Open-Meteo WMO weather code
String getWeatherDesc( int code );

// Returns a compass bearing abbreviation (N/NE/E/… ) for a wind direction in degrees
String getWindDir( int deg );

// Fetches weather data from Open-Meteo (geocoding + forecast) and updates all
// global weather state variables.  Calls detectTimezoneFromCoords internally.
void fetchWeatherData();
