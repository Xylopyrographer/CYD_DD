#include "backlight.h"
#include "led.h"
#include <TFT_eSPI.h>   // provides TFT_BL from User_Setup.h
#include <Arduino.h>

// Externs defined in main.cpp
extern bool autoDimEnabled;
extern int  autoDimStart;
extern int  autoDimEnd;
extern int  autoDimLevel;
extern bool isDimmed;
extern int  brightness;

// ---------------------------------------------------------------------------
void backlightInit( int initialBrightness ) {
    // ESP32 Arduino 3.x pin-based LEDC API:
    // ledcAttach( pin, freq_hz, resolution_bits )
    ledcAttach( TFT_BL, BL_PWM_FREQ_HZ, BL_PWM_RES_BITS );
    backlightSet( initialBrightness );
}

void backlightSet( int value ) {
    ledcWrite( TFT_BL, value );
}

// Brightness level before auto-dim kicks in, so we restore the user's chosen level
static int preDimBrightness = 255;

// ---------------------------------------------------------------------------
void applyAutoDim() {
    if ( !autoDimEnabled ) {
        return;
    }

    time_t now = time( nullptr );
    struct tm *timeinfo = localtime( &now );
    int currentHour = timeinfo->tm_hour;

    bool shouldDim = false;
    if ( autoDimStart < autoDimEnd ) {
        // Normal interval (e.g. 22-6 is not normal, that crosses midnight)
        shouldDim = ( currentHour >= autoDimStart && currentHour < autoDimEnd );
    }
    else {
        // Overnight interval (e.g. start=22, end=6)
        shouldDim = ( currentHour >= autoDimStart || currentHour < autoDimEnd );
    }

    if ( shouldDim && !isDimmed ) {
        // Capture user's current brightness before dimming
        preDimBrightness = brightness;
        brightness = map( autoDimLevel, 0, 100, 0, 255 );
        isDimmed = true;
        backlightSet( brightness );
        log_d( "[AUTODIM] Dim ON - level: %d", brightness );
    }
    else if ( !shouldDim && isDimmed ) {
        // Restore brightness to what it was before dimming
        brightness = preDimBrightness;
        isDimmed = false;
        backlightSet( brightness );
        log_d( "[AUTODIM] Dim OFF - brightness: %d", brightness );
    }
}

