#pragma once
#include <Arduino.h>

// ================= BUILT-IN RGB LED PIN DEFINITIONS =================
// The CYD has three discrete LEDs (R, G, B) in a single package — NOT a WS2812.
// All three are active-LOW: write 0 to turn ON, 1 to turn OFF.
#define CYD_LED_RED     4
#define CYD_LED_GREEN   16
#define CYD_LED_BLUE    17

// ================= CONVENIENCE MACROS FOR LED CONTROL =================
#define CYD_LED_RED_OFF()   ( digitalWrite( CYD_LED_RED,   1 ) )
#define CYD_LED_RED_ON()    ( digitalWrite( CYD_LED_RED,   0 ) )
#define CYD_LED_GREEN_OFF() ( digitalWrite( CYD_LED_GREEN, 1 ) )
#define CYD_LED_GREEN_ON()  ( digitalWrite( CYD_LED_GREEN, 0 ) )
#define CYD_LED_BLUE_OFF()  ( digitalWrite( CYD_LED_BLUE,  1 ) )
#define CYD_LED_BLUE_ON()   ( digitalWrite( CYD_LED_BLUE,  0 ) )

#define CYD_LED_RGB_OFF()  do { CYD_LED_RED_OFF();  CYD_LED_GREEN_OFF(); CYD_LED_BLUE_OFF(); } while(0)
#define CYD_LED_RGB_ON()   do { CYD_LED_RED_ON();   CYD_LED_GREEN_ON();  CYD_LED_BLUE_ON();  } while(0)

// ================= LCD BACKLIGHT =================
// Backlight pin is TFT_BL (21), defined in include/User_Setup.h via TFT_eSPI.
#define CYD_LED_WHITE_OFF() CYD_LED_RGB_OFF()
#define CYD_LED_WHITE_ON()  CYD_LED_RGB_ON()

// Initialise all three LED GPIO pins and turn them off.
inline void initLEDS( void ) {
    pinMode( CYD_LED_RED,   OUTPUT );
    pinMode( CYD_LED_GREEN, OUTPUT );
    pinMode( CYD_LED_BLUE,  OUTPUT );
    CYD_LED_RGB_OFF();
}
