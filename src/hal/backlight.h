#pragma once
#include <Arduino.h>

// LEDC PWM settings for TFT backlight
constexpr uint32_t BL_PWM_FREQ_HZ  = 5000;   // 5 kHz — above audible range
constexpr uint8_t  BL_PWM_RES_BITS = 8;      // 8-bit duty (0–255)

// Call once in setup() — attaches TFT_BL to the LEDC peripheral
void backlightInit( int initialBrightness = 255 );

// Set backlight duty cycle (0–255)
void backlightSet( int value );

// Apply scheduled auto-dim (call every ~60 s from the main loop)
void applyAutoDim();

// Cancel auto-dim immediately and restore normal brightness (e.g. on any touch)
void backlightCancelDim();
