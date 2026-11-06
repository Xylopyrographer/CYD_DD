#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// Panel colour compensation: this panel physically swaps the red/blue
// channels (calibration-verified: RED drawn → BLUE shown). Passing the
// desired colour as 0xRRGGBB returns the RGB565 value the hardware must
// receive so the eye sees exactly that colour. Use for ALL WOOD-theme
// colours — direct draws and sprite pixels alike.
inline uint16_t panelColor( uint32_t rgb888 ) {
    uint32_t r = ( rgb888 >> 16 ) & 0xFF;
    uint32_t g = ( rgb888 >>  8 ) & 0xFF;
    uint32_t b =  rgb888        & 0xFF;
    return ( uint16_t )( ( ( b >> 3 ) << 11 ) | ( ( g >> 2 ) << 5 ) | ( r >> 3 ) );
}

// Returns the background colour for the active theme
uint16_t getBgColor();

// Returns the primary text colour for the active theme
uint16_t getTextColor();

// Returns the seconds-hand colour for the active theme
uint16_t getSecHandColor();

// --- THEME_WOOD palette -----------------------------------------------------
// Brass accents used across the vintage face. Safe to query in any theme;
// they only meaningful for THEME_WOOD but return valid colours regardless.
uint16_t getBrassColor();      // main brass tone (bezel ring, accents)
uint16_t getBrassDarkColor();  // shaded brass (separator lines, inner ring)
uint16_t getDialColor();       // cream analog dial face
uint16_t getInkColor();        // warm near-black for numerals / hands on dial

// Draws the procedural wood-grain background into the given rectangle.
// Deterministic (pure function of y), so repeated partial redraws always
// reproduce the exact same pattern — no shimmer between refreshes.
void drawWoodTexture( int x, int y, int w, int h );

// Draws the dark walnut picture-frame border around the whole screen
// (vintage look). Cheap — a handful of rects, safe to redraw anytime.
void drawWoodFrame();
