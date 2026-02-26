#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// --- Weather icon primitives ---
void drawCloudVector( int x, int y, uint32_t color );
void drawWeatherIconVector( int code, int x, int y );
void drawWeatherIconVectorSmall( int code, int x, int y );
void drawMoonPhaseIcon( int mx, int my, int r, int phase, uint16_t textColor, uint16_t bgColor );

// --- Status indicators ---
void drawWifiIndicator();
void drawUpdateIndicator();

// --- Navigation / settings icons ---
void drawSettingsIcon( uint16_t color );
void drawArrowBack( int x, int y, uint16_t color );
void drawArrowDown( int x, int y, uint16_t color );
void drawArrowUp( int x, int y, uint16_t color );

// --- Drawing primitives ---
void fillGradientVertical( int x, int y, int w, int h, uint16_t colorTop, uint16_t colorBottom );
void drawDegreeCircle( int x, int y, int r, uint16_t color );
