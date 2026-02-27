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
void redrawBrightnessSlider();   // partial repaint — slider + % label only
void drawInitialSetup();

// --- Keyboard ---
void drawKeyboardScreen();
void updateKeyboardText();
void handleKeyboardTouch( int x, int y );
