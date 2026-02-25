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
    return TFT_WHITE;
}

uint16_t getSecHandColor() {
    if ( themeMode == THEME_BLUE ) {
        return yellowLight;    // Second hand in blue theme = yellow
    }
    if ( themeMode == THEME_YELLOW ) {
        return blueLight;    // Second hand in yellow theme = blue
    }
    return isWhiteTheme ? TFT_RED : TFT_YELLOW;
}
