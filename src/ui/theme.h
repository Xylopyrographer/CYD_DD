#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// Returns the background colour for the active theme
uint16_t getBgColor();

// Returns the primary text colour for the active theme
uint16_t getTextColor();

// Returns the seconds-hand colour for the active theme
uint16_t getSecHandColor();
