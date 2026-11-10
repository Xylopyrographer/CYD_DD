#include "theme.h"
#include "../util/constants.h"

// ---------------------------------------------------------------------------
// Externs – defined in main.cpp
// ---------------------------------------------------------------------------
extern int      themeMode;
extern bool     isWhiteTheme;
extern uint16_t blueDark;
extern uint16_t blueLight;
extern uint16_t yellowDark;
extern uint16_t yellowLight;

extern TFT_eSPI tft;   // defined in main.cpp

// ---------------------------------------------------------------------------

uint16_t getBgColor() {
    if ( themeMode == THEME_DARK ) {
        return isWhiteTheme ? TFT_WHITE : TFT_BLACK;
    }
    if ( themeMode == THEME_WHITE ) {
        return isWhiteTheme ? TFT_WHITE : TFT_BLACK;
    }
    if ( themeMode == THEME_BLUE ) {
        return blueDark;    // BLUE - dark background
    }
    if ( themeMode == THEME_YELLOW ) {
        return yellowDark;    // YELLOW - dark background
    }
    if ( themeMode == THEME_WOOD ) {
        return panelColor( 0x4A2E12 );        // #4A2E12 dark walnut
    }
    if ( themeMode == THEME_TECH ) {
        return panelColor( 0x0B0D10 );        // near-black tech background
    }
    if ( themeMode == THEME_CARD ) {
        return panelColor( 0xF4EDDD );        // warm cream card background
    }
    return TFT_BLACK;
}

uint16_t getTextColor() {
    if ( themeMode == THEME_DARK ) {
        return isWhiteTheme ? TFT_BLACK : TFT_WHITE;
    }
    if ( themeMode == THEME_WHITE ) {
        return isWhiteTheme ? TFT_BLACK : TFT_WHITE;
    }
    if ( themeMode == THEME_BLUE ) {
        return blueLight;    // BLUE - light text
    }
    if ( themeMode == THEME_YELLOW ) {
        return yellowLight;    // YELLOW - light text
    }
    if ( themeMode == THEME_WOOD ) {
        return panelColor( 0xF5EBD8 );         // aged cream
    }
    if ( themeMode == THEME_TECH ) {
        return panelColor( 0xF2F5F7 );         // crisp white
    }
    if ( themeMode == THEME_CARD ) {
        return panelColor( 0x1F3A66 );         // deep navy
    }
    return TFT_WHITE;
}

uint16_t getSecHandColor() {
    if ( themeMode == THEME_BLUE ) {
        return yellowLight;    // Second hand in blue theme = yellow
    }
    if ( themeMode == THEME_YELLOW ) {
        return blueLight;    // Second hand in yellow theme = blue
    }
    if ( themeMode == THEME_WOOD ) {
        return panelColor( 0x8B1E1E );         // deep red lacquer hand
    }
    return isWhiteTheme ? TFT_RED : TFT_YELLOW;
}

// ---------------------------------------------------------------------------
// THEME_WOOD palette — vintage wood / brass face
// ---------------------------------------------------------------------------
uint16_t getBrassColor() {
    return panelColor( 0xC9A24B );             // brushed brass
}

uint16_t getBrassDarkColor() {
    return panelColor( 0x8C6B2F );             // shaded brass
}

uint16_t getDialColor() {
    return panelColor( 0xF2E8D0 );             // aged cream dial face
}

uint16_t getInkColor() {
    return panelColor( 0x201408 );             // warm near-black ink
}

// Deterministic wood grain — pure function of the row index y. Every partial
// redraw (weather panel, date block) reproduces identical rows, so patches
// painted over the background blend seamlessly with the original full-screen
// texture.
uint16_t woodRowColor( int y ) {
    // Base tone oscillates very subtly between dark walnut shades — the mockup
    // shows a smooth, almost-solid dark wood, not visible stripes.
    float t = sinf( y * 0.21f ) * 0.6f + sinf( y * 0.07f + 1.3f ) * 0.4f;  // -1..1
    int d = ( int )( t * 3.0f );                                           // -3..3

    // Occasional faint grain lines (barely visible, deterministic pattern)
    if ( ( y * 13 ) % 37 < 3 ) {
        d -= 2;
    }

    uint16_t base = getBgColor();
    int r = ( base >> 11 ) & 0x1F;
    int g = ( base >> 5 ) & 0x3F;
    int b = base & 0x1F;
    r = constrain( r + d / 2, 0, 31 );
    g = constrain( g + d, 0, 63 );
    b = constrain( b + d / 2, 0, 31 );
    return ( uint16_t )( ( r << 11 ) | ( g << 5 ) | b );
}

void drawWoodTexture( int x, int y, int w, int h ) {
    for ( int yy = y; yy < y + h; yy++ ) {
        tft.drawFastHLine( x, yy, w, woodRowColor( yy ) );
    }
}

// Picture-frame border around the whole screen (vintage look)
void drawWoodFrame() {
    const uint16_t frameDark = panelColor( 0x3A2410 );   // near-black walnut
    const uint16_t frameMid  = panelColor( 0x5D3A1A );   // mid walnut
    // 6 px deep frame
    for ( int i = 0; i < 6; i++ ) {
        uint16_t c = ( i < 3 ) ? frameDark : frameMid;
        tft.drawRect( i, i, 320 - 2 * i, 240 - 2 * i, c );
    }
    // Thin brass fillet just inside the frame
    tft.drawRect( 7, 7, 320 - 14, 240 - 14, getBrassDarkColor() );
}
