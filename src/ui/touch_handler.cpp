#include "touch_handler.h"
#include "theme.h"
#include "icons.h"
#include "clock_face.h"
#include "screens.h"
#include "../hal/backlight.h"

#include "../hal/led.h"

#include <WiFi.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <time.h>

#include "../util/constants.h"
#include "../data/app_state.h"
#include "../data/city_data.h"
#include "../data/nameday.h"
#include "../net/ota.h"
#include "../net/weather_api.h"

// ---------------------------------------------------------------------------
// Externs — all defined in main.cpp
// ---------------------------------------------------------------------------
extern TFT_eSPI   tft;
extern bool       isWhiteTheme;
extern int        themeMode;
extern uint16_t   blueLight;
extern uint16_t   blueDark;
extern uint16_t   yellowLight;
extern uint16_t   yellowDark;

// Layout
extern const int  MENU_BASE_Y;
extern const int  MENU_ITEM_HEIGHT;
extern const int  MENU_ITEM_GAP;
extern const int  MENU_ITEM_SPACING;
extern int        menuOffset;
extern int        countryOffset;
extern int        cityOffset;

// WiFi
extern const int  MAX_NETWORKS;
extern String     wifiSSIDs[];
extern int        wifiCount;
extern int        wifiOffset;
extern String     ssid;
extern String     password;
extern String     selectedSSID;
extern String     passwordBuffer;
extern bool       keyboardNumbers;
extern bool       keyboardShift;
extern bool       showPassword;

// Location / region
extern String     selectedCountry;
extern String     selectedCity;
extern String     selectedTimezone;
extern String     posixTZ;
extern String     lookupCountry;
extern String     lookupCity;
extern String     lookupTimezone;
extern int        lookupGmtOffset;
extern int        lookupDstOffset;
extern float      lat;
extern float      lon;
extern String     cityName;
extern String     countryName;
extern bool       regionAutoMode;
extern RecentCity recentCities[];
extern int        recentCount;
extern String     customCityInput;
extern String     customCountryInput;
extern String     weatherCity;
extern String     timezoneName;

// Weather units
extern bool       weatherUnitF;
extern bool       weatherUnitMph;
extern bool       weatherUnitInHg;

// OTA / firmware
extern const char *FIRMWARE_VERSION;
extern String     availableVersion;
extern String     downloadURL;
extern bool       updateAvailable;
extern int        otaInstallMode;
extern bool       isUpdating;
extern int        updateProgress;
extern String     updateStatus;
extern unsigned long lastVersionCheck;

// Graphics / autodim
extern int        brightness;
extern bool       autoDimEnabled;
extern int        autoDimStart;
extern int        autoDimEnd;
extern int        autoDimLevel;
extern bool       invertColors;
extern int        autoDimEditMode;
extern int        autoDimTempStart;
extern int        autoDimTempEnd;
extern int        autoDimTempLevel;

// Clock / time
extern bool       isDigitalClock;
extern bool       is12hFormat;
extern long       gmtOffset_sec;
extern int        daylightOffset_sec;
extern int        lastSec;
extern int        lastDay;
extern bool       forceClockRedraw;
extern unsigned long lastWeatherUpdate;

// Coordinate edit
extern String     coordLatBuffer;
extern String     coordLonBuffer;
extern bool       coordEditingLon;

// Lookup buffers
extern float      lookupLat;
extern float      lookupLon;

// NTP
extern const char *ntpServer;

// State / prefs
extern ScreenState currentState;
extern Preferences prefs;

// Forward declarations for functions that remain in main.cpp
void   applyLocation();
bool   getTimezoneForCity( String countryName, String city, String &timezone,
                           int &gmt, int &dst );
bool   lookupCountryGeonames( String countryName );
bool   lookupCityGeonames( String cityName, String countryHint );
void   getCountryCities( String countryName, String cities[], int &count );
String obfuscatePassword( const String &plain );
void   syncRegion();
void   addToRecentCities( String city, String country, String timezone,
                          int gmtOffset, int dstOffset );

// ---------------------------------------------------------------------------
void handleTouch( int x, int y ) {
    switch ( currentState ) {
        case CLOCK: {
            // Settings button
            if ( x >= 270 && x <= 320 && y >= 200 && y <= 240 ) {
                currentState = SETTINGS;
                menuOffset = 0;
                drawSettingsScreen();
            }

            // Touch on clock area to toggle 12/24h format (DIGITAL MODE ONLY)
            // Clock area approx x: 180-280, y: 40-130 (based on clockX, clockY, radius)
            // clockX = 230, clockY = 85, radius = 67
            if ( isDigitalClock && x >= 160 && x <= 300 && y >= 20 && y <= 150 ) {
                is12hFormat = !is12hFormat;
                prefs.begin( "sys", false );
                prefs.putBool( "12hFmt", is12hFormat );
                prefs.end();
                // Force redraw by clearing lastSec
                lastSec = -1;
                delay( TOUCH_DEBOUNCE_MS );
            }
            break;
        }

        case SETTINGS: {
            // Back button (red)
            if ( x >= 230 && x <= 280 && y >= 125 && y <= 175 ) {
                currentState = CLOCK;
                lastSec = -1;
                delay( UI_DEBOUNCE_MS );
            }
            // Up arrow
            else if ( menuOffset > 0 && x >= 230 && x <= 280 && y >= 70 && y <= 120 ) {
                menuOffset--;
                drawSettingsScreen();
                delay( UI_DEBOUNCE_MS );
            }
            // Down arrow
            else if ( menuOffset < 2 && x >= 230 && x <= 280 && y >= 180 && y <= 230 ) {
                menuOffset++;
                drawSettingsScreen();
                delay( UI_DEBOUNCE_MS );
            }
            // Detect taps on menu items
            else {
                for ( int i = 0; i < 4; i++ ) { // 4 visible items on screen
                    if ( isTouchInMenuItem( y, i ) ) {
                        int actualItem = i + menuOffset;  // Remap: visual position → actual item

                        switch ( actualItem ) {
                            case 0: // WiFi Setup
                                currentState = WIFICONFIG;
                                scanWifiNetworks();
                                wifiOffset = 0;
                                drawInitialSetup();
                                break;

                            case 1: // Weather
                                currentState = WEATHERCONFIG;
                                drawWeatherScreen();
                                break;

                            case 2: // Regional
                                currentState = REGIONALCONFIG;
                                drawRegionalScreen();
                                break;

                            case 3: // Graphics
                                currentState = GRAPHICSCONFIG;
                                drawGraphicsScreen();
                                break;

                            case 4: // Firmware
                                currentState = FIRMWARE_SETTINGS;
                                drawFirmwareScreen();
                                break;

                            case 5: // Calibrate
                                runTouchCalibration();
                                menuOffset = 0;
                                drawSettingsScreen();
                                break;
                        }

                        delay( UI_DEBOUNCE_MS );
                        break;  // Exit for loop
                    }
                }
            }
            break;
        }

        case WIFICONFIG: {
            if ( ssid != "" && x >= 265 && x <= 315 && y >= 50 && y <= 100 ) {
                currentState = SETTINGS;
                menuOffset = 0;
                drawSettingsScreen();
            }
            else if ( x >= 265 && x <= 315 && y >= 110 && y <= 160 ) {
                if ( wifiOffset > 0 ) {
                    wifiOffset--;
                    drawInitialSetup();
                }
            }
            else if ( x >= 265 && x <= 315 && y >= 170 && y <= 220 ) {
                if ( wifiOffset + 6 <= wifiCount ) {
                    wifiOffset++;
                    drawInitialSetup();
                }
            }
            else {
                bool handled = false;
                for ( int i = wifiOffset; i < wifiOffset + 6 && i < wifiCount; i++ ) {
                    int idx = i - wifiOffset;
                    int yPos = 45 + idx * 30;
                    if ( y >= yPos && y <= yPos + 25 ) {
                        selectedSSID = wifiSSIDs[ i ];
                        currentState = KEYBOARD;
                        passwordBuffer = "";
                        keyboardNumbers = false;
                        keyboardShift = false;
                        drawKeyboardScreen();
                        handled = true;
                        break;
                    }
                }
                // Check tap on "Other..." row
                if ( !handled ) {
                    int otherVisIdx = wifiCount - wifiOffset;
                    if ( otherVisIdx >= 0 && otherVisIdx < 6 ) {
                        int yPos = 45 + otherVisIdx * 30;
                        if ( y >= yPos && y <= yPos + 25 ) {
                            selectedSSID = "";
                            currentState = SSID_INPUT;
                            passwordBuffer = "";
                            keyboardNumbers = false;
                            keyboardShift = false;
                            drawKeyboardScreen();
                        }
                    }
                }
            }
            break;
        }

        case SSID_INPUT: {
            // Same key handling as KEYBOARD
            for ( int r = 0; r < 3; r++ ) {
                const char *row;
                if ( keyboardNumbers ) {
                    if ( r == 0 ) {
                        row = "1234567890";
                    }
                    else if ( r == 1 ) {
                        row = "!@#$%^&*(/";
                    }
                    else {
                        row = ")-_+=.,?";
                    }
                }
                else {
                    if ( r == 0 ) {
                        row = "qwertyuiop";
                    }
                    else if ( r == 1 ) {
                        row = "asdfghjkl";
                    }
                    else {
                        row = "zxcvbnm";
                    }
                }
                int len = strlen( row );
                for ( int i = 0; i < len; i++ ) {
                    int btnX = i * 29 + 2;
                    int btnY = 80 + r * 30;
                    if ( x >= btnX && x <= btnX + 26 && y >= btnY && y <= btnY + 26 ) {
                        char ch = row[ i ];
                        if ( keyboardShift && !keyboardNumbers ) {
                            ch = toupper( ch );
                        }
                        passwordBuffer += ch;
                        updateKeyboardText();
                        delay( UI_DEBOUNCE_MS );
                        return;
                    }
                }
            }
            if ( x >= 2 && x <= 318 && y >= 170 && y <= 195 ) {
                passwordBuffer += " ";
                updateKeyboardText();
                delay( UI_DEBOUNCE_MS );
                return;
            }
            int bw = 64;
            int by = 198;
            int bh = 35;
            if ( x >= 0 && x <= bw && y >= by && y <= by + bh ) {
                keyboardShift = !keyboardShift;
                drawKeyboardScreen();
                delay( UI_DEBOUNCE_MS );
                return;
            }
            if ( x >= bw && x <= 2 * bw && y >= by && y <= by + bh ) {
                keyboardNumbers = !keyboardNumbers;
                drawKeyboardScreen();
                delay( UI_DEBOUNCE_MS );
                return;
            }
            if ( x >= 2 * bw && x <= 3 * bw && y >= by && y <= by + bh ) {
                if ( passwordBuffer.length() > 0 ) {
                    passwordBuffer.remove( passwordBuffer.length() - 1 );
                    updateKeyboardText();
                    delay( UI_DEBOUNCE_MS );
                }
                return;
            }
            // Back – return to WiFi list
            if ( x >= 3 * bw && x <= 4 * bw && y >= by && y <= by + bh ) {
                passwordBuffer = "";
                currentState = WIFICONFIG;
                drawInitialSetup();
                delay( TOUCH_DEBOUNCE_MS );
                return;
            }
            // OK – confirm SSID, move to password entry
            if ( x >= 4 * bw && x <= 5 * bw && y >= by && y <= by + bh ) {
                selectedSSID = passwordBuffer;
                passwordBuffer = "";
                keyboardNumbers = false;
                keyboardShift = false;
                currentState = KEYBOARD;
                drawKeyboardScreen();
                delay( TOUCH_DEBOUNCE_MS );
                return;
            }
            break;
        }

        case KEYBOARD: {
            for ( int r = 0; r < 3; r++ ) {
                const char *row;
                if ( keyboardNumbers ) {
                    if ( r == 0 ) {
                        row = "1234567890";
                    }
                    else if ( r == 1 ) {
                        row = "!@#$%^&*(/";
                    }
                    else {
                        row = ")-_+=.,?";
                    }
                }
                else {
                    if ( r == 0 ) {
                        row = "qwertyuiop";
                    }
                    else if ( r == 1 ) {
                        row = "asdfghjkl";
                    }
                    else {
                        row = "zxcvbnm";
                    }
                }
                int len = strlen( row );
                for ( int i = 0; i < len; i++ ) {
                    int btnX = i * 29 + 2;
                    int btnY = 80 + r * 30;
                    if ( x >= btnX && x <= btnX + 26 && y >= btnY && y <= btnY + 26 ) {
                        char ch = row[ i ];
                        if ( keyboardShift && !keyboardNumbers ) {
                            ch = toupper( ch );
                        }
                        passwordBuffer += ch;
                        updateKeyboardText();
                        delay( UI_DEBOUNCE_MS );
                        return;
                    }
                }
            }
            if ( x >= 2 && x <= 318 && y >= 170 && y <= 195 ) {
                passwordBuffer += " ";
                updateKeyboardText();
                delay( UI_DEBOUNCE_MS );
                return;
            }
            int bw = 64;
            int by = 198;
            int bh = 35;
            if ( x >= 0 && x <= bw && y >= by && y <= by + bh ) {
                keyboardShift = !keyboardShift;
                drawKeyboardScreen();
                delay( UI_DEBOUNCE_MS );
                return;
            }
            if ( x >= bw && x <= 2 * bw && y >= by && y <= by + bh ) {
                keyboardNumbers = !keyboardNumbers;
                drawKeyboardScreen();
                delay( UI_DEBOUNCE_MS );
                return;
            }
            if ( x >= 2 * bw && x <= 3 * bw && y >= by && y <= by + bh ) {
                if ( passwordBuffer.length() > 0 ) {
                    passwordBuffer.remove( passwordBuffer.length() - 1 );
                    updateKeyboardText();
                    delay( UI_DEBOUNCE_MS );
                }
                return;
            }
            if ( x >= 3 * bw && x <= 4 * bw && y >= by && y <= by + bh ) {
                passwordBuffer = "";
                currentState = WIFICONFIG;
                drawInitialSetup();
                delay( TOUCH_DEBOUNCE_MS );
                return;
            }
            if ( x >= 4 * bw && x <= 5 * bw && y >= by && y <= by + bh ) {
                prefs.begin( "sys", false );
                prefs.putString( "ssid", selectedSSID );
                prefs.putString( "pass", obfuscatePassword( passwordBuffer ) );
                prefs.end();
                ssid = selectedSSID;
                password = passwordBuffer;
                showWifiConnectingScreen( ssid );
                WiFi.mode( WIFI_STA );
                WiFi.disconnect();
                delay( 100 );
                WiFi.begin( ssid.c_str(), password.c_str() );
                unsigned long startWait = millis();
                while ( WiFi.status() != WL_CONNECTED && millis() - startWait < WIFI_CONNECT_TIMEOUT ) {
                    delay( 500 );
                }
                if ( WiFi.status() == WL_CONNECTED ) {
                    showWifiResultScreen( true );
                    if ( regionAutoMode ) {
                        syncRegion();
                    }
                    currentState = CLOCK;
                    lastSec = -1;
                }
                else {
                    showWifiResultScreen( false );
                    currentState = WIFICONFIG;
                    drawInitialSetup();
                }
                delay( TOUCH_DEBOUNCE_MS );
                return;
            }
            if ( x >= 250 && x <= 310 && y >= 140 && y <= 165 ) {
                showPassword = !showPassword;
                drawKeyboardScreen();
                delay( UI_DEBOUNCE_MS );
                return;
            }
            break;
        }

        case WEATHERCONFIG: {
            // ===== SLOUPEC 1: TEPLOTA =====
            // °C button: x=8 w=38 → x=8..46
            if ( x >= 8 && x <= 46 && y >= 80 && y <= 100 ) {
                if ( weatherUnitF ) {
                    weatherUnitF = false;
                    prefs.begin( "sys", false );
                    prefs.putBool( "weatherUnitF", weatherUnitF );
                    prefs.end();
                    drawWeatherScreen();
                    delay( TOUCH_DEBOUNCE_MS );
                }
                break;
            }
            // °F button: x=50 w=38 → x=50..88
            if ( x >= 50 && x <= 88 && y >= 80 && y <= 100 ) {
                if ( !weatherUnitF ) {
                    weatherUnitF = true;
                    prefs.begin( "sys", false );
                    prefs.putBool( "weatherUnitF", weatherUnitF );
                    prefs.end();
                    drawWeatherScreen();
                    delay( TOUCH_DEBOUNCE_MS );
                }
                break;
            }

            // ===== COLUMN 2: WIND =====
            // km/h button: x=115 w=38 → x=115..153
            if ( x >= 115 && x <= 153 && y >= 80 && y <= 100 ) {
                if ( weatherUnitMph ) {
                    weatherUnitMph = false;
                    prefs.begin( "sys", false );
                    prefs.putBool( "weatherUnitMph", weatherUnitMph );
                    prefs.end();
                    drawWeatherScreen();
                    delay( TOUCH_DEBOUNCE_MS );
                }
                break;
            }
            // mph button: x=157 w=38 → x=157..195
            if ( x >= 157 && x <= 195 && y >= 80 && y <= 100 ) {
                if ( !weatherUnitMph ) {
                    weatherUnitMph = true;
                    prefs.begin( "sys", false );
                    prefs.putBool( "weatherUnitMph", weatherUnitMph );
                    prefs.end();
                    drawWeatherScreen();
                    delay( TOUCH_DEBOUNCE_MS );
                }
                break;
            }

            // ===== SLOUPEC 3: TLAK =====
            // hPa button: x=222 w=38 → x=222..260
            if ( x >= 222 && x <= 260 && y >= 80 && y <= 100 ) {
                if ( weatherUnitInHg ) {
                    weatherUnitInHg = false;
                    prefs.begin( "sys", false );
                    prefs.putBool( "weatherUnitInHg", weatherUnitInHg );
                    prefs.end();
                    drawWeatherScreen();
                    delay( TOUCH_DEBOUNCE_MS );
                }
                break;
            }
            // inHg button: x=264 w=38 → x=264..302
            if ( x >= 264 && x <= 302 && y >= 80 && y <= 100 ) {
                if ( !weatherUnitInHg ) {
                    weatherUnitInHg = true;
                    prefs.begin( "sys", false );
                    prefs.putBool( "weatherUnitInHg", weatherUnitInHg );
                    prefs.end();
                    drawWeatherScreen();
                    delay( TOUCH_DEBOUNCE_MS );
                }
                break;
            }

            // ===== EDIT COORDINATES =====
            if ( x >= 232 && x <= 292 && y >= 110 && y <= 126 ) {
                coordLatBuffer = String( lat, 4 );
                coordLonBuffer = String( lon, 4 );
                coordEditingLon = false;
                currentState = COORDSINPUT;
                drawCoordInputScreen();
                delay( TOUCH_DEBOUNCE_MS );
                break;
            }

            // ===== BACK =====
            if ( x >= 40 && x <= 280 && y >= 152 && y <= 168 ) {
                currentState = SETTINGS;
                menuOffset = 0;
                drawSettingsScreen();
            }
            break;
        }
        case COORDSINPUT: {
            int by = 165;
            int bh = 35;
            int bw = 75;

            // ===== KEYBOARD - numbers mode =====
            const char *rows[] = {"1234567890", "!@#$%^&*(/", ")-_+=.,?"};
            for ( int r = 0; r < 3; r++ ) {
                int len = strlen( rows[ r ] );
                for ( int i = 0; i < len; i++ ) {
                    int btnX = i * 29 + 2;
                    int btnY = 65 + r * 30;
                    if ( x >= btnX && x <= btnX + 26 && y >= btnY && y <= btnY + 26 ) {
                        char ch = rows[ r ][ i ];
                        if ( !coordEditingLon ) {
                            coordLatBuffer += ch;
                        }
                        else {
                            coordLonBuffer += ch;
                        }
                        drawCoordInputScreen();
                        delay( 100 );
                        return;
                    }
                }
            }

            // ===== DEL =====
            if ( x >= 5 && x <= 5 + bw - 5 && y >= by && y <= by + bh ) {
                if ( !coordEditingLon ) {
                    if ( coordLatBuffer.length() > 0 ) {
                        coordLatBuffer.remove( coordLatBuffer.length() - 1 );
                    }
                }
                else {
                    if ( coordLonBuffer.length() > 0 ) {
                        coordLonBuffer.remove( coordLonBuffer.length() - 1 );
                    }
                }
                drawCoordInputScreen();
                delay( 100 );
                return;
            }

            // ===== LAT/LON TOGGLE =====
            if ( x >= bw + 5 && x <= bw + 5 + bw - 5 && y >= by && y <= by + bh ) {
                coordEditingLon = !coordEditingLon;
                drawCoordInputScreen();
                delay( UI_DEBOUNCE_MS );
                return;
            }

            // ===== SAVE =====
            if ( x >= 2 * bw + 5 && x <= 2 * bw + 5 + bw - 5 && y >= by && y <= by + bh ) {
                float newLat = coordLatBuffer.toFloat();
                float newLon = coordLonBuffer.toFloat();
                if ( newLat != 0.0 || newLon != 0.0 ) {
                    lat = newLat;
                    lon = newLon;
                    lookupLat = lat;
                    lookupLon = lon;
                    prefs.begin( "sys", false );
                    prefs.putFloat( "lat", lat );
                    prefs.putFloat( "lon", lon );
                    prefs.end();
                    lastWeatherUpdate = 0; // Force weather update with new coordinates
                    log_i( "[COORDS] Manual coordinates saved: %.4f, %.4f", lat, lon );
                }
                currentState = WEATHERCONFIG;
                drawWeatherScreen();
                delay( TOUCH_DEBOUNCE_MS );
                return;
            }

            // ===== BACK =====
            if ( x >= 3 * bw + 5 && x <= 3 * bw + 5 + bw - 5 && y >= by && y <= by + bh ) {
                currentState = WEATHERCONFIG;
                drawWeatherScreen();
                delay( UI_DEBOUNCE_MS );
                return;
            }
            break;
        }
        case REGIONALCONFIG: {
            if ( x >= 160 - 55 && x <= 160 + 55 && y >= 60 - 15 && y <= 60 + 15 ) {
                regionAutoMode = !regionAutoMode;
                prefs.begin( "sys", false );
                prefs.putBool( "regionAuto", regionAutoMode );
                prefs.end();
                drawRegionalScreen();
            }
            else if ( !regionAutoMode && x >= 120 && x <= 148 && y >= 172 && y <= 188 ) {
                gmtOffset_sec += 3600;
                if ( gmtOffset_sec > 50400 ) {
                    gmtOffset_sec = 50400;
                }
                daylightOffset_sec = 0;
                int posixOff = -( gmtOffset_sec / 3600 );
                posixTZ = "UTC" + String( posixOff );
                configTime( 0, 0, ntpServer );
                setenv( "TZ", posixTZ.c_str(), 1 );
                tzset();
                prefs.begin( "sys", false );
                prefs.putInt( "gmt", gmtOffset_sec );
                prefs.putInt( "dst", daylightOffset_sec );
                prefs.putString( "posixTZ", posixTZ );
                prefs.end();
                drawRegionalScreen();
                delay( UI_DEBOUNCE_MS );
            }
            else if ( !regionAutoMode && x >= 155 && x <= 183 && y >= 172 && y <= 188 ) {
                gmtOffset_sec -= 3600;
                if ( gmtOffset_sec < -43200 ) {
                    gmtOffset_sec = -43200;
                }
                daylightOffset_sec = 0;
                int posixOff = -( gmtOffset_sec / 3600 );
                posixTZ = "UTC" + String( posixOff );
                configTime( 0, 0, ntpServer );
                setenv( "TZ", posixTZ.c_str(), 1 );
                tzset();
                prefs.begin( "sys", false );
                prefs.putInt( "gmt", gmtOffset_sec );
                prefs.putInt( "dst", daylightOffset_sec );
                prefs.putString( "posixTZ", posixTZ );
                prefs.end();
                drawRegionalScreen();
                delay( UI_DEBOUNCE_MS );
            }
            else if ( x >= 40 && x <= 145 && y >= 205 && y <= 235 ) {
                if ( regionAutoMode ) {
                    syncRegion();
                    currentState = CLOCK;
                    lastSec = -1;
                }
                else {
                    currentState = COUNTRYSELECT;
                    countryOffset = 0;
                    drawCountrySelection();
                }
            }
            else if ( x >= 155 && x <= 260 && y >= 205 && y <= 235 ) {
                currentState = CLOCK;
                lastSec = -1;
            }
            break;
        }

        case COUNTRYSELECT: {
            if ( x >= 230 && x <= 320 && y >= 45 && y <= 95 ) {
                if ( countryOffset > 0 ) {
                    countryOffset--;
                }
                drawCountrySelection();
            }
            else if ( x >= 230 && x <= 320 && y >= 180 && y <= 230 ) {
                if ( countryOffset + 5 < COUNTRIES_COUNT ) {
                    countryOffset++;
                }
                drawCountrySelection();
            }
            else if ( x >= 230 && x <= 320 && y >= 110 && y <= 160 ) {
                currentState = REGIONALCONFIG;
                drawRegionalScreen();
            }
            else if ( y >= 70 + 5 * 30 && y <= 70 + 6 * 30 ) {
                customCountryInput = "";
                currentState = CUSTOMCOUNTRYINPUT;
                keyboardNumbers = false;
                keyboardShift = false;
                drawCustomCountryInput();
            }
            else {
                for ( int i = countryOffset; i < countryOffset + 5 && i < COUNTRIES_COUNT; i++ ) {
                    int idx = i - countryOffset;
                    int yPos = 70 + idx * 30;
                    if ( y >= yPos && y <= yPos + 25 ) {
                        selectedCountry = String( countries[ i ].name );
                        currentState = CITYSELECT;
                        cityOffset = 0;
                        drawCitySelection();
                        break;
                    }
                }
            }
            break;
        }

        case CITYSELECT: {
            String cities[ 20 ];
            int cityCount = 0;
            getCountryCities( selectedCountry, cities, cityCount );
            if ( x >= 230 && x <= 320 && y >= 45 && y <= 95 ) {
                if ( cityOffset > 0 ) {
                    cityOffset--;
                }
                drawCitySelection();
            }
            else if ( x >= 230 && x <= 320 && y >= 180 && y <= 230 ) {
                if ( cityOffset + 5 < cityCount ) {
                    cityOffset++;
                }
                drawCitySelection();
            }
            else if ( x >= 230 && x <= 320 && y >= 110 && y <= 160 ) {
                currentState = COUNTRYSELECT;
                countryOffset = 0;
                drawCountrySelection();
            }
            else if ( y >= 70 + 5 * 30 && y <= 70 + 6 * 30 ) {
                customCityInput = "";
                currentState = CUSTOMCITYINPUT;
                keyboardNumbers = false;
                keyboardShift = false;
                drawCustomCityInput();
            }
            else {
                for ( int i = cityOffset; i < cityOffset + 5 && i < cityCount; i++ ) {
                    int idx = i - cityOffset;
                    int yPos = 70 + idx * 30;
                    if ( y >= yPos && y <= yPos + 25 ) {
                        selectedCity = cities[ i ];
                        String tz;
                        int go, doff;
                        if ( getTimezoneForCity( selectedCountry, selectedCity, tz, go, doff ) ) {
                            selectedTimezone = tz;
                            gmtOffset_sec = go;
                            daylightOffset_sec = doff;
                            currentState = LOCATIONCONFIRM;
                            drawLocationConfirm();
                        }
                        break;
                    }
                }
            }
            break;
        }

        case LOCATIONCONFIRM: {
            if ( x >= 40 && x <= 145 && y >= 205 && y <= 235 ) {
                applyLocation();
                currentState = CLOCK;
                lastSec = -1;
            }
            else if ( x >= 155 && x <= 260 && y >= 205 && y <= 235 ) {
                currentState = CITYSELECT;
                cityOffset = 0;
                drawCitySelection();
            }
            break;
        }

        case CUSTOMCITYINPUT: {
            if ( x >= 180 && x <= 250 && y >= 198 && y <= 233 ) {
                if ( customCityInput.length() > 0 ) {
                    lookupCityGeonames( customCityInput, selectedCountry );
                    currentState = CITYLOOKUPCONFIRM;
                    drawCityLookupConfirm();
                }
                else {
                    drawCustomCityInput();
                }
                return;
            }

            // FIX: Selecting the correct character set by keyboardNumbers
            const char *rows[ 3 ];
            if ( keyboardNumbers ) {
                rows[ 0 ] = "1234567890";
                rows[ 1 ] = "!@#$%^&*(/";
                rows[ 2 ] = ")-_+=.,?";
            }
            else {
                rows[ 0 ] = "qwertyuiop";
                rows[ 1 ] = "asdfghjkl";
                rows[ 2 ] = "zxcvbnm";
            }

            for ( int r = 0; r < 3; r++ ) {
                for ( int i = 0; i < strlen( rows[ r ] ); i++ ) {
                    if ( x >= i * 29 && x <= i * 29 + 29 && y >= 80 + r * 30 && y <= 80 + r * 30 + 30 ) {
                        char ch = rows[ r ][ i ];
                        if ( keyboardShift && !keyboardNumbers ) {
                            ch = toupper( ch );
                        }
                        customCityInput += ch;
                        keyboardShift = false;
                        drawCustomCityInput();
                        return;
                    }
                }
            }
            if ( x >= 0 && x <= 318 && y >= 170 && y <= 195 ) {
                customCityInput += " ";
                drawCustomCityInput();
                return;
            }
            if ( x >= 250 && x <= 320 && y >= 198 && y <= 233 ) {
                customCityInput = "";
                currentState = CITYSELECT;
                cityOffset = 0;
                drawCitySelection();
                return;
            }
            if ( x >= 120 && x <= 180 && y >= 198 && y <= 233 ) {
                if ( customCityInput.length() > 0 ) {
                    customCityInput.remove( customCityInput.length() - 1 );
                    drawCustomCityInput();
                }
                return;
            }
            if ( x >= 60 && x <= 120 && y >= 198 && y <= 233 ) {
                keyboardNumbers = !keyboardNumbers;
                drawCustomCityInput();
                return;
            }
            if ( x >= 0 && x <= 60 && y >= 198 && y <= 233 ) {
                keyboardShift = !keyboardShift;
                drawCustomCityInput();
                return;
            }
            break;
        }

        case CUSTOMCOUNTRYINPUT: {
            if ( x >= 180 && x <= 250 && y >= 198 && y <= 233 ) {
                if ( customCountryInput.length() > 0 ) {
                    lookupCountryGeonames( customCountryInput );
                    currentState = COUNTRYLOOKUPCONFIRM;
                    drawCountryLookupConfirm();
                }
                else {
                    drawCustomCountryInput();
                }
                return;
            }

            // FIX: Select correct character set based on keyboardNumbers
            const char *rows[ 3 ];
            if ( keyboardNumbers ) {
                rows[ 0 ] = "1234567890";
                rows[ 1 ] = "!@#$%^&*(/";
                rows[ 2 ] = ")-_+=.,?";
            }
            else {
                rows[ 0 ] = "qwertyuiop";
                rows[ 1 ] = "asdfghjkl";
                rows[ 2 ] = "zxcvbnm";
            }

            for ( int r = 0; r < 3; r++ ) {
                for ( int i = 0; i < strlen( rows[ r ] ); i++ ) {
                    if ( x >= i * 29 && x <= i * 29 + 29 && y >= 80 + r * 30 && y <= 80 + r * 30 + 30 ) {
                        char ch = rows[ r ][ i ];
                        if ( keyboardShift && !keyboardNumbers ) {
                            ch = toupper( ch );
                        }
                        customCountryInput += ch;
                        keyboardShift = false;
                        drawCustomCountryInput();
                        return;
                    }
                }
            }
            if ( x >= 0 && x <= 318 && y >= 170 && y <= 195 ) {
                customCountryInput += " ";
                drawCustomCountryInput();
                return;
            }
            if ( x >= 250 && x <= 320 && y >= 198 && y <= 233 ) {
                customCountryInput = "";
                currentState = COUNTRYSELECT;
                countryOffset = 0;
                drawCountrySelection();
                return;
            }
            if ( x >= 120 && x <= 180 && y >= 198 && y <= 233 ) {
                if ( customCountryInput.length() > 0 ) {
                    customCountryInput.remove( customCountryInput.length() - 1 );
                    drawCustomCountryInput();
                }
                return;
            }
            if ( x >= 60 && x <= 120 && y >= 198 && y <= 233 ) {
                keyboardNumbers = !keyboardNumbers;
                drawCustomCountryInput();
                return;
            }
            if ( x >= 0 && x <= 60 && y >= 198 && y <= 233 ) {
                keyboardShift = !keyboardShift;
                drawCustomCountryInput();
                return;
            }
            break;
        }

        case CITYLOOKUPCONFIRM: {
            if ( x >= 40 && x <= 145 && y >= 205 && y <= 235 ) {
                selectedCity = lookupCity;
                selectedTimezone = lookupTimezone;
                gmtOffset_sec = lookupGmtOffset;
                daylightOffset_sec = lookupDstOffset;
                applyLocation();
                // applyLocation() resets lat/lon to 0.0 - restore coordinates from Nominatim lookup
                if ( lookupLat != 0.0 || lookupLon != 0.0 ) {
                    lat = lookupLat;
                    lon = lookupLon;
                    prefs.begin( "sys", false );
                    prefs.putFloat( "lat", lat );
                    prefs.putFloat( "lon", lon );
                    prefs.end();
                    log_d( "[LOOKUP] Restored coordinates: %.4f, %.4f", lat, lon );
                }
                currentState = CLOCK;
                lastSec = -1;
            }
            else if ( x >= 155 && x <= 260 && y >= 205 && y <= 235 ) {
                currentState = CITYSELECT;
                drawCitySelection();
            }
            break;
        }

        case COUNTRYLOOKUPCONFIRM: {
            if ( x >= 40 && x <= 145 && y >= 205 && y <= 235 ) {
                selectedCountry = lookupCountry;
                currentState = CITYSELECT;
                cityOffset = 0;
                drawCitySelection();
            }
            else if ( x >= 155 && x <= 260 && y >= 205 && y <= 235 ) {
                currentState = COUNTRYSELECT;
                countryOffset = 0;
                drawCountrySelection();
            }
            break;
        }
        case FIRMWARE_SETTINGS: {
            // Back button (unified position matching other menus)
            if ( x >= 230 && x <= 280 && y >= 125 && y <= 175 ) {
                currentState = SETTINGS;
                menuOffset = 0;
                drawSettingsScreen();
                delay( UI_DEBOUNCE_MS );
                break;
            }

            // Radio buttons for install mode (Auto and By user only)
            // V drawFirmwareScreen:
            // yPos = 60 (Current) => 85 (Available) => 120 (Install mode) => 145 (first radio button)
            // First radio button:  btnY = 145 + (0 * 25) = 145
            // Second radio button: btnY = 145 + (1 * 25) = 170
            for ( int i = 0; i < 2; i++ ) {
                int btnY = 145 + ( i * 25 ); // FIXED: 145 instead of 120
                // Circle centred on btnY, radius 6px
                // Touch area: wider for better UX (+/-10px from centre)
                if ( x >= 10 && x <= 30 && y >= btnY - 10 && y <= btnY + 10 ) {
                    otaInstallMode = i;
                    prefs.begin( "sys", false );
                    prefs.putInt( "otaMode", otaInstallMode );
                    prefs.end();
                    log_i( "[OTA] Install mode changed to: %s", i == 0 ? "Auto" : "By user" );
                    drawFirmwareScreen();
                    delay( UI_DEBOUNCE_MS );
                    break;
                }
            }

            // CHECK NOW / INSTALL button
            if ( x >= 10 && x <= 150 && y >= 190 && y <= 220 ) {
                if ( updateAvailable ) {
                    // INSTALL - always perform update (Manual mode no longer exists)
                    performOTAUpdate();
                }
                else {
                    // CHECK NOW
                    tft.fillScreen( TFT_BLACK );
                    tft.setTextColor( TFT_WHITE );
                    tft.setTextDatum( MC_DATUM );
                    tft.drawString( "CHECKING...", 160, 100, 2 );

                    checkForUpdate();

                    delay( 1000 );
                    drawFirmwareScreen();
                }
                delay( UI_DEBOUNCE_MS );
                break;
            }
            break;
        }

        case GRAPHICSCONFIG: {
            // ... (Theme code stays the same) ...
            if ( x >= 20 && x <= 70 && y >= 65 && y <= 95 ) {
                themeMode = THEME_DARK;
                isWhiteTheme = false;
                prefs.begin( "sys", false );
                prefs.putInt( "themeMode", themeMode );
                prefs.putBool( "theme", isWhiteTheme );
                prefs.end();
                tft.fillScreen( getBgColor() );
                drawGraphicsScreen();
                delay( TOUCH_DEBOUNCE_MS );
                break;
            }
            if ( x >= 80 && x <= 130 && y >= 65 && y <= 95 ) {
                themeMode = THEME_WHITE;
                isWhiteTheme = true;
                prefs.begin( "sys", false );
                prefs.putInt( "themeMode", themeMode );
                prefs.putBool( "theme", isWhiteTheme );
                prefs.end();
                tft.fillScreen( getBgColor() );
                drawGraphicsScreen();
                delay( TOUCH_DEBOUNCE_MS );
                break;
            }
            if ( x >= 140 && x <= 190 && y >= 65 && y <= 95 ) {
                themeMode = THEME_BLUE;
                prefs.begin( "sys", false );
                prefs.putInt( "themeMode", themeMode );
                prefs.end();
                fillGradientVertical( 0, 0, 320, 240, blueDark, blueLight );
                drawGraphicsScreen();
                delay( TOUCH_DEBOUNCE_MS );
                break;
            }
            if ( x >= 200 && x <= 250 && y >= 65 && y <= 95 ) {
                themeMode = THEME_YELLOW;
                prefs.begin( "sys", false );
                prefs.putInt( "themeMode", themeMode );
                prefs.end();
                fillGradientVertical( 0, 0, 320, 240, yellowDark, yellowLight );
                drawGraphicsScreen();
                delay( TOUCH_DEBOUNCE_MS );
                break;
            }

            // === NEW INVERT BUTTON ===
            if ( x >= 260 && x <= 310 && y >= 65 && y <= 95 ) {
                log_d( "[INVERT] Toggle: %s -> %s", invertColors ? "TRUE" : "FALSE", !invertColors ? "TRUE" : "FALSE" );
                invertColors = !invertColors;

                bool prefOpened = prefs.begin( "sys", false );
                if ( prefOpened ) {
                    size_t written = prefs.putBool( "invertColors", invertColors );
                    delay( 100 ); // Give extra time for the write to complete
                    prefs.end();

                    // VERIFY: Re-open and read back
                    prefs.begin( "sys", true ); // read-only
                    bool readBack = prefs.getBool( "invertColors", false );
                    prefs.end();
                    log_d( "[INVERT] Written: %u bytes, readback: %s, match: %d", ( unsigned )written, readBack ? "TRUE" : "FALSE", readBack == invertColors );
                }

                // ILI9341 (CYD1): invertColors directly controls inversion.
                // invertColors=false → tft.invertDisplay(false) = normal display
                // invertColors=true  → tft.invertDisplay(true)  = inverted display
                tft.invertDisplay( invertColors );
                log_d( "[INVERT] Display SW inverted: %s", invertColors ? "TRUE" : "FALSE" );

                drawGraphicsScreen();
                delay( TOUCH_DEBOUNCE_MS );
                break;
            }

            // === NEW BRIGHTNESS CONTROL (COMPACT) ===
            // Slider is now at x=10, width=130
            if ( x >= 10 && x <= 140 && y >= 125 && y <= 137 ) {
                int newBrightness = map( x - 10, 0, 130, 0, 255 );
                brightness = constrain( newBrightness, 0, 255 );
                // Throttle NVS writes to ≤1 per 500 ms — flash writes can stall the bus
                static unsigned long lastNVSSaveBright = 0;
                if ( millis() - lastNVSSaveBright > 500 ) {
                    prefs.begin( "sys", false );
                    prefs.putInt( "bright", brightness );
                    prefs.end();
                    lastNVSSaveBright = millis();
                }
                backlightSet( brightness );
                redrawBrightnessSlider();   // partial repaint — no fillScreen flash
                break;
            }

            // === NEW ANALOG / DIGITAL TOGGLE ===
            // Oblast: x >= 200, y cca 115-143
            if ( x >= 200 && x <= 310 && y >= 115 && y <= 145 ) {
                isDigitalClock = !isDigitalClock;
                prefs.begin( "sys", false );
                prefs.putBool( "digiClock", isDigitalClock );
                prefs.end();
                drawGraphicsScreen();
                delay( TOUCH_DEBOUNCE_MS );
                break;
            }

            // Back button
            if ( x >= 252 && x <= 308 && y >= 182 && y <= 238 ) {
                currentState = SETTINGS;
                menuOffset = 0;
                drawSettingsScreen();
                delay( UI_DEBOUNCE_MS );
                break;
            }

            // ... (Rest of AutoDim code stays the same) ...
            if ( x >= 10 && x <= 38 && y >= 175 && y <= 191 ) {
                // ... AutoDim ON code ...
                autoDimEnabled = true;
                prefs.begin( "sys", false );
                prefs.putBool( "autoDimEnabled", autoDimEnabled );
                prefs.end();
                drawGraphicsScreen();
                delay( UI_DEBOUNCE_MS );
                break;
            }
            if ( x >= 10 && x <= 38 && y >= 195 && y <= 211 ) {
                // ... AutoDim OFF code ...
                autoDimEnabled = false;
                prefs.begin( "sys", false );
                prefs.putBool( "autoDimEnabled", autoDimEnabled );
                prefs.end();
                drawGraphicsScreen();
                delay( UI_DEBOUNCE_MS );
                break;
            }
            // ... (zbytek AutoDim logiky ponech beze změny) ...
            // COPY THE ENTIRE AUTODIM BLOCK FROM THE ORIGINAL FILE IF UNCERTAIN.
            // This just indicates that the rest of case GRAPHICSCONFIG is unchanged

            // AUTODIM LOGIC CONTINUATION (for completeness in copy-paste block):
            if ( autoDimEnabled ) {
                int startX = 50;
                int startY = 178;
                int lineHeight = 16;
                int startTimeX = startX + 50;
                int startPlusX = startTimeX + 50;
                int startMinusX = startPlusX + 26;
                int btnW = 16;
                if ( x >= startPlusX && x <= startPlusX + btnW && y >= startY - 6 && y <= startY + 6 ) {
                    autoDimStart = ( autoDimStart + 1 ) % 24;
                    prefs.begin( "sys", false );
                    prefs.putInt( "autoDimStart", autoDimStart );
                    prefs.end();
                    drawGraphicsScreen();
                    delay( UI_DEBOUNCE_MS );
                    break;
                }
                if ( x >= startMinusX && x <= startMinusX + btnW && y >= startY - 6 && y <= startY + 6 ) {
                    autoDimStart = ( autoDimStart - 1 + 24 ) % 24;
                    prefs.begin( "sys", false );
                    prefs.putInt( "autoDimStart", autoDimStart );
                    prefs.end();
                    drawGraphicsScreen();
                    delay( UI_DEBOUNCE_MS );
                    break;
                }
                int endY = startY + lineHeight;
                int endPlusX = startTimeX + 50;
                int endMinusX = endPlusX + 26;
                if ( x >= endPlusX && x <= endPlusX + btnW && y >= endY - 6 && y <= endY + 6 ) {
                    autoDimEnd = ( autoDimEnd + 1 ) % 24;
                    prefs.begin( "sys", false );
                    prefs.putInt( "autoDimEnd", autoDimEnd );
                    prefs.end();
                    drawGraphicsScreen();
                    delay( UI_DEBOUNCE_MS );
                    break;
                }
                if ( x >= endMinusX && x <= endMinusX + btnW && y >= endY - 6 && y <= endY + 6 ) {
                    autoDimEnd = ( autoDimEnd - 1 + 24 ) % 24;
                    prefs.begin( "sys", false );
                    prefs.putInt( "autoDimEnd", autoDimEnd );
                    prefs.end();
                    drawGraphicsScreen();
                    delay( UI_DEBOUNCE_MS );
                    break;
                }
                int levelY = endY + lineHeight;
                int levelPlusX = startTimeX + 50;
                int levelMinusX = levelPlusX + 26;
                if ( x >= levelPlusX && x <= levelPlusX + btnW && y >= levelY - 6 && y <= levelY + 6 ) {
                    autoDimLevel = min( autoDimLevel + 5, 100 );
                    prefs.begin( "sys", false );
                    prefs.putInt( "autoDimLevel", autoDimLevel );
                    prefs.end();
                    drawGraphicsScreen();
                    delay( UI_DEBOUNCE_MS );
                    break;
                }
                if ( x >= levelMinusX && x <= levelMinusX + btnW && y >= levelY - 6 && y <= levelY + 6 ) {
                    autoDimLevel = max( autoDimLevel - 5, 0 );
                    prefs.begin( "sys", false );
                    prefs.putInt( "autoDimLevel", autoDimLevel );
                    prefs.end();
                    drawGraphicsScreen();
                    delay( UI_DEBOUNCE_MS );
                    break;
                }
            }
            break;
        }
    }
}
