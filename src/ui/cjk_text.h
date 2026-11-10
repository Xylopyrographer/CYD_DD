#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// Drop-in replacement for tft.drawString(): identical signature and datum /
// colour behaviour, but strings containing non-ASCII characters are rendered
// with the bundled CJK bitmap subset (see cjk_font.h). Pure-ASCII strings are
// delegated to tft.drawString() unchanged, so numeric / latin rendering keeps
// its original look and metrics.
void cjkDraw( const String &text, int32_t x, int32_t y );
void cjkDraw( const String &text, int32_t x, int32_t y, uint8_t font );

// Pixel width the string will occupy (native font for ASCII, CJK subset otherwise)
int cjkMeasure( const String &text, uint8_t font );
