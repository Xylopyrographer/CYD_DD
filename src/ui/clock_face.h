#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>

void drawClockStatic();
void drawClockFace();
void drawDateAndWeek( const struct tm *ti );
void drawDigitalClock( int h, int m, int s );
void updateHands( int h, int m, int s );
void drawWeatherSection();
