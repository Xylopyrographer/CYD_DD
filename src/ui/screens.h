#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// --- Layout helpers ---
int  getMenuItemY( int itemIndex );
bool isTouchInMenuItem( int y, int itemIndex );

// --- WiFi / startup screens ---
void showWifiConnectingScreen( String ssid );
void showWifiResultScreen( bool success );
void drawLoadingScreen();              // Static loading screen shown while NTP syncs
void drawSyncOverlay( const String &msg, bool okButton );  // Modal overlay for SYNC feedback
void drawRegionalDstButton();   // Repaint only the DST toggle button (no fillScreen)
void clearSyncOverlay();        // Erase sync overlay and restore the content beneath it
void redrawBrightnessSlider();  // Repaint only the brightness slider (no fillScreen)
void redrawAutoDimLevel();      // Repaint only the auto-dim level value (no fillScreen)
void redrawAutoDimStart();      // Repaint only the auto-dim start time value
void redrawAutoDimEnd();        // Repaint only the auto-dim end time value
void redrawAutoDimSection();    // Repaint the entire auto-dim subpanel (no fillScreen)
void redrawDigiAnaToggle();     // Repaint only the ANA/DIGI toggle widget
void scanWifiNetworks();

// --- Touch calibration ---
void runTouchCalibration();

// --- Settings screens ---
void drawSettingsScreen();
void drawWeatherScreen();
void drawCoordInputScreen();
void drawRegionalScreen();
void drawCountrySelection();
void drawCitySelection();
void drawLocationConfirm();
void drawCountryLookupConfirm();
void drawCityLookupConfirm();
void drawCustomCityInput();
void drawCustomCountryInput();
void drawFirmwareScreen();
void drawGraphicsScreen();
void drawInitialSetup();

// --- Keyboard ---
void drawKeyboardScreen();
void updateKeyboardText();
void handleKeyboardTouch( int x, int y );
