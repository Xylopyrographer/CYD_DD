#include "screens.h"
#include "theme.h"
#include "icons.h"

#include <WiFi.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <XPT2046_Touchscreen.h>

#include "../util/constants.h"
#include "../data/app_state.h"
#include "../data/city_data.h"
#include "../data/nameday.h"
#include "../net/ota.h"

// ---------------------------------------------------------------------------
// Externs - defined in main.cpp
// ---------------------------------------------------------------------------
extern TFT_eSPI tft;
extern bool     isWhiteTheme;
extern int      themeMode;
extern uint16_t blueLight;
extern uint16_t blueDark;
extern uint16_t yellowLight;
extern uint16_t yellowDark;

// Layout
extern const int MENU_BASE_Y;
extern const int MENU_ITEM_HEIGHT;
extern const int MENU_ITEM_GAP;
extern const int MENU_ITEM_SPACING;
extern int menuOffset;
extern int countryOffset;
extern int cityOffset;

// WiFi
extern const int MAX_NETWORKS;
extern String    wifiSSIDs[];
extern int       wifiCount;
extern int       wifiOffset;
extern String    ssid;
extern String    password;
extern String    selectedSSID;
extern String    passwordBuffer;
extern bool      keyboardNumbers;
extern bool      keyboardShift;
extern bool      showPassword;

// Location / region
extern String    selectedCountry;
extern String    selectedCity;
extern String    selectedTimezone;
extern String    posixTZ;
extern String    lookupCountry;
extern String    lookupCity;
extern String    lookupTimezone;
extern int       lookupGmtOffset;
extern int       lookupDstOffset;
extern float     lat;
extern float     lon;
extern String    cityName;
extern String    countryName;
extern bool      regionAutoMode;
extern RecentCity recentCities[];
extern int       recentCount;
extern String    customCityInput;
extern String    customCountryInput;
extern String    weatherCity;

// Weather units
extern bool weatherUnitF;
extern bool weatherUnitMph;
extern bool weatherUnitInHg;

// OTA / firmware
extern const char *FIRMWARE_VERSION;
extern String      availableVersion;
extern String      downloadURL;
extern bool        updateAvailable;
extern int         otaInstallMode;
extern bool        isUpdating;
extern int         updateProgress;
extern String      updateStatus;
extern unsigned long lastVersionCheck;

// Graphics / autodim
extern int  brightness;
extern bool autoDimEnabled;
extern int  autoDimStart;
extern int  autoDimEnd;
extern int  autoDimLevel;

// Touch calibration
extern XPT2046_Touchscreen ts;
extern int touchXMin;
extern int touchXMax;
extern int touchYMin;
extern int touchYMax;
extern int touchXMinF;
extern int touchXMaxF;
extern int touchYMinF;
extern int touchYMaxF;
extern bool invertColors;
extern bool displayFlipped;

// State / prefs
extern ScreenState currentState;
extern Preferences prefs;

// Clock / digital clock state
extern bool isDigitalClock;
extern long gmtOffset_sec;
extern int  lastSec;

// Coordinate edit buffers
extern String coordLatBuffer;
extern String coordLonBuffer;
extern bool   coordEditingLon;

// Forward declarations for functions that remain in main.cpp
void   applyLocation();
bool   lookupCountryGeonames( String countryName );
bool   lookupCityGeonames( String cityName, String countryHint );
void   getCountryCities( String countryName, String cities[], int &count );
String obfuscatePassword( const String &plain );
void   syncRegion();

// ---------------------------------------------------------------------------

void showWifiConnectingScreen( String ssid ) {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Connecting to", 160, 80, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    tft.drawString( ssid, 160, 110, 2 );
    tft.setTextColor( getTextColor() );
    tft.drawString( "Please wait...", 160, 150, 2 );
}

void showWifiResultScreen( bool success ) {
    tft.fillScreen( getBgColor() );
    if ( success ) {
        tft.setTextColor( TFT_GREEN );
        tft.setTextDatum( MC_DATUM );
        tft.drawString( "Connection Successful!", 160, 100, 2 );
    }
    else {
        tft.setTextColor( TFT_RED );
        tft.setTextDatum( MC_DATUM );
        tft.drawString( "Connection FAILED", 160, 100, 2 );
    }
    delay( UI_SPLASH_DELAY_MS );
}

void scanWifiNetworks() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Scanning WiFi...", 160, 120, 2 );

    WiFi.mode( WIFI_STA );
    if ( WiFi.status() != WL_CONNECTED ) {
        WiFi.disconnect( false );
    }

    int n = WiFi.scanNetworks();
    wifiCount = 0;

    if ( n > 0 ) {
        for ( int i = 0; i < n && wifiCount < MAX_NETWORKS; i++ ) {
            String entry = WiFi.SSID( i );
            if ( entry.length() > 0 ) {
                wifiSSIDs[ wifiCount++ ] = entry;
            }
        }
    }
    log_d( "[WIFI] Scan complete. Found %d networks", wifiCount );
    WiFi.scanDelete();
}

// ---------------------------------------------------------------------------
// Touch calibration
// Draw a 2-point calibration procedure and save results to NVS.
// Reads raw ADC values from XPT2046 (bypassing the current mapping) and
// computes new TOUCH_X_MIN/MAX / TOUCH_Y_MIN/MAX via linear extrapolation.
// ---------------------------------------------------------------------------
static void drawCalTarget( int cx, int cy, uint16_t color ) {
    tft.drawFastHLine( cx - 12, cy,      24, color );
    tft.drawFastVLine( cx,      cy - 12, 24, color );
    tft.drawCircle( cx, cy, 5, color );
}

static TS_Point waitCalTouch() {
    // Drain any held touch first
    while ( ts.touched() ) {
        delay( 10 );
    }
    // Wait for a new touch with sufficient pressure
    TS_Point p;
    do {
        while ( !ts.touched() ) {
            delay( 10 );
        }
        delay( 40 );   // brief settle
        p = ts.getPoint();
    } while ( p.z < 200 );
    while ( ts.touched() ) {
        delay( 10 );    // wait for release
    }
    return p;
}

void runTouchCalibration() {
    const uint16_t BG = TFT_BLACK;
    const uint16_t FG = TFT_WHITE;
    const int T1X = 20,  T1Y = 20;    // target 1 screen position
    const int T2X = 300, T2Y = 220;   // target 2 screen position

    // --- Step 1 ---
    tft.fillScreen( BG );
    tft.setTextColor( FG, BG );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Touch Calibration", 160, 80, 2 );
    tft.drawString( "Tap the cross", 160, 110, 2 );
    drawCalTarget( T1X, T1Y, TFT_YELLOW );
    delay( 300 );

    TS_Point p1 = waitCalTouch();
    log_d( "[CAL] Point 1 raw: x=%d y=%d", p1.x, p1.y );

    // --- Step 2 ---
    tft.fillScreen( BG );
    tft.setTextColor( FG, BG );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Touch Calibration", 160, 80, 2 );
    tft.drawString( "Tap the cross", 160, 110, 2 );
    drawCalTarget( T2X, T2Y, TFT_YELLOW );
    delay( 300 );

    TS_Point p2 = waitCalTouch();
    log_d( "[CAL] Point 2 raw: x=%d y=%d", p2.x, p2.y );

    // --- Compute new mapping ---
    // Linear extrapolation so that:
    //   map(rawAtT1, xMin, xMax, 0, 320) == T1X
    //   map(rawAtT2, xMin, xMax, 0, 320) == T2X
    float scaleX = ( float )( p2.x - p1.x ) / ( float )( T2X - T1X );
    float scaleY = ( float )( p2.y - p1.y ) / ( float )( T2Y - T1Y );

    int newXMin = ( int )( p1.x - T1X * scaleX );
    int newXMax = ( int )( p1.x + ( 320 - T1X ) * scaleX );
    int newYMin = ( int )( p1.y - T1Y * scaleY );
    int newYMax = ( int )( p1.y + ( 240 - T1Y ) * scaleY );

    log_d( "[CAL] New mapping: X %d-%d  Y %d-%d", newXMin, newXMax, newYMin, newYMax );

    // --- Apply immediately ---
    touchXMin = newXMin;
    touchXMax = newXMax;
    touchYMin = newYMin;
    touchYMax = newYMax;

    // --- Persist to NVS in orientation-specific keyset ---
    prefs.begin( "sys", false );
    if ( displayFlipped ) {
        prefs.putInt( "calXMinF", newXMin );
        prefs.putInt( "calXMaxF", newXMax );
        prefs.putInt( "calYMinF", newYMin );
        prefs.putInt( "calYMaxF", newYMax );
    }
    else {
        prefs.putInt( "calXMin",  newXMin );
        prefs.putInt( "calXMax",  newXMax );
        prefs.putInt( "calYMin",  newYMin );
        prefs.putInt( "calYMax",  newYMax );
    }
    prefs.end();

    // --- Done ---
    tft.fillScreen( BG );
    tft.setTextColor( TFT_GREEN, BG );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Calibration saved!", 160, 110, 2 );
    tft.drawString( "Returning to settings...", 160, 135, 2 );
    delay( 1500 );
}

void drawCoordInputScreen() {
    uint16_t bg = getBgColor();
    uint16_t txt = getTextColor();
    tft.fillScreen( bg );

    tft.setTextDatum( MC_DATUM );
    tft.setTextColor( txt, bg );
    tft.drawString( "MANUAL COORDINATES", 160, 18, 2 );

    uint16_t latBorderCol = !coordEditingLon ? TFT_SKYBLUE : TFT_DARKGREY;
    uint16_t lonBorderCol =  coordEditingLon ? TFT_SKYBLUE : TFT_DARKGREY;
    tft.setTextDatum( ML_DATUM );
    tft.setTextColor( !coordEditingLon ? TFT_SKYBLUE : txt, bg );
    tft.drawString( "Lat:", 5, 42, 2 );
    tft.drawRect( 40, 33, 120, 18, latBorderCol );
    tft.setTextColor( !coordEditingLon ? TFT_SKYBLUE : txt, bg );
    tft.drawString( coordLatBuffer, 44, 43, 1 );

    tft.setTextColor( coordEditingLon ? TFT_SKYBLUE : txt, bg );
    tft.drawString( "Lon:", 175, 42, 2 );
    tft.drawRect( 210, 33, 105, 18, lonBorderCol );
    tft.setTextColor( coordEditingLon ? TFT_SKYBLUE : txt, bg );
    tft.drawString( coordLonBuffer, 214, 43, 1 );

    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextDatum( MC_DATUM );
    const char *rows[] = {"1234567890", "!@#$%^&*(/", ")-_+=.,?"};
    for ( int r = 0; r < 3; r++ ) {
        int len = strlen( rows[ r ] );
        for ( int i = 0; i < len; i++ ) {
            int btnX = i * 29 + 2;
            int btnY = 65 + r * 30;
            tft.drawRect( btnX, btnY, 26, 26, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
            tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );
            tft.drawString( String( rows[ r ][ i ] ), btnX + 13, btnY + 15 );
        }
    }

    tft.setFreeFont( NULL );
    tft.setTextDatum( MC_DATUM );
    int bw = 75;
    int by = 165;
    int bh = 35;

    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );
    tft.drawRect( 5, by, bw - 5, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "DEL", 5 + ( bw - 5 ) / 2, by + 18 );

    tft.setTextColor( TFT_SKYBLUE );
    tft.drawRect( bw + 5, by, bw - 5, bh, TFT_SKYBLUE );
    tft.drawString( !coordEditingLon ? "LON >" : "< LAT", bw + 5 + ( bw - 5 ) / 2, by + 18 );

    tft.setTextColor( TFT_GREEN );
    tft.drawRect( 2 * bw + 5, by, bw - 5, bh, TFT_GREEN );
    tft.drawString( "SAVE", 2 * bw + 5 + ( bw - 5 ) / 2, by + 18 );

    tft.setTextColor( TFT_ORANGE );
    tft.drawRect( 3 * bw + 5, by, bw - 5, bh, TFT_ORANGE );
    tft.drawString( "BACK", 3 * bw + 5 + ( bw - 5 ) / 2, by + 18 );

    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );
}

void drawCountryLookupConfirm() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "COUNTRY FOUND", 160, 30, 4 );

    tft.setTextDatum( ML_DATUM );
    tft.drawString( "Country", 40, 80, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    tft.drawString( lookupCountry, 40, 100, 2 );
    tft.setTextColor( getTextColor() );
    tft.drawString( "Status", 40, 140, 2 );
    tft.setTextColor( TFT_GREEN );
    tft.drawString( "CONFIRMED", 40, 160, 2 );

    tft.setTextDatum( MC_DATUM );
    tft.drawRoundRect( 40, 205, 105, 30, 6, TFT_GREEN );
    tft.drawString( "NEXT", 92, 220, 2 );
    tft.drawRoundRect( 155, 205, 105, 30, 6, TFT_RED );
    tft.drawString( "Back", 207, 220, 2 );
}

void drawCityLookupConfirm() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "CITY FOUND", 160, 30, 4 );

    tft.setTextDatum( ML_DATUM );
    tft.drawString( "City", 40, 80, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    tft.drawString( lookupCity, 40, 100, 2 );
    tft.setTextColor( getTextColor() );
    tft.drawString( "Timezone", 40, 130, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    tft.drawString( lookupTimezone, 40, 150, 2 );
    tft.setTextColor( getTextColor() );
    tft.drawString( "Status", 40, 180, 2 );
    tft.setTextColor( TFT_GREEN );
    tft.drawString( "CONFIRMED", 40, 200, 2 );

    tft.setTextDatum( MC_DATUM );
    tft.drawRoundRect( 40, 205, 105, 30, 6, TFT_GREEN );
    tft.drawString( "SAVE", 92, 220, 2 );
    tft.drawRoundRect( 155, 205, 105, 30, 6, TFT_RED );
    tft.drawString( "Back", 207, 220, 2 );
}

void drawCustomCountryInput() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.setFreeFont( &FreeSansBold12pt7b );
    tft.drawString( "Enter Country Name", 160, 20 );

    tft.drawRect( 10, 40, 300, 30, isWhiteTheme ? TFT_BLACK : TFT_WHITE );
    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextDatum( ML_DATUM );
    tft.drawString( customCountryInput, 20, 55 );

    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextDatum( MC_DATUM );

    const char *rows[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    if ( keyboardNumbers ) {
        rows[ 0 ] = "1234567890";
        rows[ 1 ] = "!@#$%^&*(/";
        rows[ 2 ] = ")-_+=.,?";
    }

    for ( int r = 0; r < 3; r++ ) {
        int len = strlen( rows[ r ] );
        for ( int i = 0; i < len; i++ ) {
            int btnX = i * 29 + 2;
            int btnY = 80 + r * 30;
            tft.drawRect( btnX, btnY, 26, 26, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
            char ch = rows[ r ][ i ];
            if ( keyboardShift && !keyboardNumbers ) {
                ch = toupper( ch );
            }
            tft.drawString( String( ch ), btnX + 13, btnY + 15 );
        }
    }

    tft.drawRect( 2, 170, 316, 25, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Space", 160, 183 );

    int bw = 64;
    int by = 198;
    int bh = 35;
    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );

    tft.drawRect( 0 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "Shift", 0 * bw + bw / 2, by + 18 );

    tft.drawRect( 1 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "123", 1 * bw + bw / 2, by + 18 );

    tft.drawRect( 2 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "Del", 2 * bw + bw / 2, by + 18 );

    tft.setTextColor( TFT_GREEN );
    tft.drawRect( 3 * bw + 2, by, bw - 4, bh, TFT_GREEN );
    tft.drawString( "SRCH", 3 * bw + bw / 2, by + 18 );

    tft.setTextColor( TFT_ORANGE );
    tft.drawRect( 4 * bw + 2, by, bw - 4, bh, TFT_ORANGE );
    tft.drawString( "BACK", 4 * bw + bw / 2, by + 18 );

    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );
}

// ---------------------------------------------------------------------------

int getMenuItemY( int itemIndex ) {
    return MENU_BASE_Y + itemIndex * MENU_ITEM_SPACING;
}

bool isTouchInMenuItem( int y, int itemIndex ) {
    int yPos = getMenuItemY( itemIndex );
    return ( y >= yPos && y <= yPos + MENU_ITEM_HEIGHT );
}

void drawSettingsScreen() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "SETTINGS", 160, 30, 4 );

    String menuItems[] = {"WiFi Setup", "Weather", "Regional", "Graphics", "Firmware", "Calibrate"};
    uint16_t colors[]  = {TFT_BLUE, TFT_BLUE, TFT_BLUE, TFT_BLUE, TFT_BLUE, TFT_ORANGE};

    int totalItems   = 6;
    int visibleItems = 4;  // How many fit on screen at once

    for ( int i = 0; i < totalItems; i++ ) {
        if ( i >= menuOffset && i < menuOffset + visibleItems ) {
            int yPos = getMenuItemY( i - menuOffset );
            tft.drawRoundRect( 40, yPos, 180, MENU_ITEM_HEIGHT, 6, colors[ i ] );
            tft.drawRoundRect( 39, yPos - 1, 182, MENU_ITEM_HEIGHT + 2, 6, colors[ i ] ); // Thick border!
            tft.fillRoundRect( 41, yPos + 1, 178, MENU_ITEM_HEIGHT - 2, 5, getBgColor() ); // Fill
            tft.drawString( menuItems[ i ], 130, yPos + 17, 2 );
        }
    }

    // Up arrow (if not at start)
    if ( menuOffset > 0 ) {
        tft.drawRoundRect( 230, 70, 50, 50, 4, TFT_BLUE );
        drawArrowUp( 230, 70, TFT_BLUE );
    }

    // Back button
    tft.drawRoundRect( 230, 125, 50, 50, 4, TFT_RED );
    drawArrowBack( 230, 125, TFT_RED );

    // Down arrow (if more than 4 items)
    if ( menuOffset < ( totalItems - visibleItems ) ) {
        tft.drawRoundRect( 230, 180, 50, 50, 4, TFT_BLUE );
        drawArrowDown( 230, 180, TFT_BLUE );
    }
}

void drawWeatherScreen() {
    uint16_t bg = getBgColor();
    uint16_t txt = getTextColor();
    tft.fillScreen( bg );

    // ===== NADPIS =====
    tft.setFreeFont( &FreeSans12pt7b );
    tft.setTextColor( TFT_ORANGE, bg );
    tft.setTextDatum( TC_DATUM );
    tft.drawString( "Weather Settings", 160, 5 );

    // ===== MĚSTO =====
    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextColor( TFT_SKYBLUE, bg );
    tft.setTextDatum( TC_DATUM );
    String cityDisp = ( cityName == "" ) ? "Not set (Use Regional)" : cityName;
    if ( cityDisp.length() > 22 ) {
        cityDisp = cityDisp.substring( 0, 19 ) + "...";
    }
    tft.drawString( cityDisp, 160, 38 );

    tft.setFreeFont( NULL );
    tft.setTextDatum( MC_DATUM );

    // ===== 3 SLOUPCE JEDNOTEK - LABELS =====
    tft.setTextColor( txt, bg );
    tft.drawString( "Temperature", 50, 68, 1 );
    tft.drawString( "Wind speed", 160, 68, 1 );
    tft.drawString( "Pressure", 270, 68, 1 );

    // ===== SLOUPEC 1: TEPLOTA =====
    int btnH = 20;
    int btnY = 80;
    if ( !weatherUnitF ) {
        tft.fillRoundRect( 8,  btnY, 38, btnH, 3, TFT_GREEN );
        tft.setTextColor( TFT_WHITE, TFT_GREEN );
        tft.drawString( "C", 27, btnY + 10, 1 );
        tft.drawRoundRect( 50, btnY, 38, btnH, 3, TFT_BLUE );
        tft.setTextColor( txt, bg );
        tft.drawString( "F", 69, btnY + 10, 1 );
    }
    else {
        tft.drawRoundRect( 8,  btnY, 38, btnH, 3, TFT_BLUE );
        tft.setTextColor( txt, bg );
        tft.drawString( "C", 27, btnY + 10, 1 );
        tft.fillRoundRect( 50, btnY, 38, btnH, 3, TFT_GREEN );
        tft.setTextColor( TFT_WHITE, TFT_GREEN );
        tft.drawString( "F", 69, btnY + 10, 1 );
    }

    // ===== COLUMN 2: WIND =====
    tft.setTextColor( txt, bg );
    if ( !weatherUnitMph ) {
        tft.fillRoundRect( 115, btnY, 38, btnH, 3, TFT_GREEN );
        tft.setTextColor( TFT_WHITE, TFT_GREEN );
        tft.drawString( "km/h", 134, btnY + 10, 1 );
        tft.drawRoundRect( 157, btnY, 38, btnH, 3, TFT_BLUE );
        tft.setTextColor( txt, bg );
        tft.drawString( "mph", 176, btnY + 10, 1 );
    }
    else {
        tft.drawRoundRect( 115, btnY, 38, btnH, 3, TFT_BLUE );
        tft.setTextColor( txt, bg );
        tft.drawString( "km/h", 134, btnY + 10, 1 );
        tft.fillRoundRect( 157, btnY, 38, btnH, 3, TFT_GREEN );
        tft.setTextColor( TFT_WHITE, TFT_GREEN );
        tft.drawString( "mph", 176, btnY + 10, 1 );
    }

    // ===== SLOUPEC 3: TLAK =====
    tft.setTextColor( txt, bg );
    if ( !weatherUnitInHg ) {
        tft.fillRoundRect( 222, btnY, 38, btnH, 3, TFT_GREEN );
        tft.setTextColor( TFT_WHITE, TFT_GREEN );
        tft.drawString( "hPa", 241, btnY + 10, 1 );
        tft.drawRoundRect( 264, btnY, 38, btnH, 3, TFT_BLUE );
        tft.setTextColor( txt, bg );
        tft.drawString( "inHg", 283, btnY + 10, 1 );
    }
    else {
        tft.drawRoundRect( 222, btnY, 38, btnH, 3, TFT_BLUE );
        tft.setTextColor( txt, bg );
        tft.drawString( "hPa", 241, btnY + 10, 1 );
        tft.fillRoundRect( 264, btnY, 38, btnH, 3, TFT_GREEN );
        tft.setTextColor( TFT_WHITE, TFT_GREEN );
        tft.drawString( "inHg", 283, btnY + 10, 1 );
    }

    // ===== COORDINATES + EDIT =====
    tft.setTextColor( txt, bg );
    tft.setTextDatum( ML_DATUM );
    String coordStr = "Coord: " + String( lat, 2 ) + ", " + String( lon, 2 );
    tft.drawString( coordStr, 8, 118, 1 );
    tft.drawRoundRect( 232, 110, 60, 16, 3, TFT_SKYBLUE );
    tft.setTextColor( TFT_SKYBLUE, bg );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "EDIT", 262, 118, 1 );

    // ===== INFO =====
    tft.setTextColor( txt, bg );
    tft.drawString( "Updates every 30 min", 160, 138, 1 );

    // ===== BACK BUTTON =====
    tft.fillRoundRect( 40, 152, 240, 16, 4, TFT_DARKGREY );
    tft.setTextColor( TFT_WHITE );
    tft.drawString( "BACK TO SETTINGS", 160, 160, 1 );
}

void drawRegionalScreen() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "REGIONAL SETUP", 160, 30, 4 );

    int toggleX = 160;
    int toggleY = 60;

    if ( regionAutoMode ) {
        tft.fillRoundRect( toggleX - 55, toggleY - 15, 50, 30, 4, TFT_GREEN );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "AUTO", toggleX - 30, toggleY, 2 );
        tft.drawRoundRect( toggleX + 5, toggleY - 15, 50, 30, 4, TFT_BLUE );
        tft.setTextColor( getTextColor() );
        tft.drawString( "MANUAL", toggleX + 30, toggleY, 2 );
    }
    else {
        tft.drawRoundRect( toggleX - 55, toggleY - 15, 50, 30, 4, TFT_BLUE );
        tft.setTextColor( getTextColor() );
        tft.drawString( "AUTO", toggleX - 30, toggleY, 2 );
        tft.fillRoundRect( toggleX + 5, toggleY - 15, 50, 30, 4, TFT_GREEN );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "MANUAL", toggleX + 30, toggleY, 2 );
    }

    tft.setTextDatum( ML_DATUM );
    tft.setTextColor( getTextColor() );
    tft.drawString( "City", 40, 110, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    tft.drawString( cityName != "" ? cityName : "---", 40, 130, 2 );

    tft.setTextColor( getTextColor() );
    tft.drawString( "Timezone", 40, 160, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    int tzHours = gmtOffset_sec / 3600;
    String tzStr = ( tzHours >= 0 ? "+" : "" ) + String( tzHours ) + "h";
    tft.drawString( tzStr, 40, 180, 2 );
    if ( !regionAutoMode ) {
        tft.setTextDatum( MC_DATUM );
        tft.drawRoundRect( 120, 172, 28, 16, 3, TFT_GREEN );
        tft.setTextColor( TFT_GREEN );
        tft.drawString( "+", 134, 180, 2 );
        tft.drawRoundRect( 155, 172, 28, 16, 3, TFT_GREEN );
        tft.drawString( "-", 169, 180, 2 );
        tft.setTextDatum( ML_DATUM );
    }

    tft.setTextDatum( MC_DATUM );
    if ( regionAutoMode ) {
        tft.drawRoundRect( 40, 205, 105, 30, 6, TFT_GREEN );
        tft.drawString( "SYNC", 92, 220, 2 );
    }
    else {
        tft.drawRoundRect( 40, 205, 105, 30, 6, TFT_GREEN );
        tft.drawString( "EDIT", 92, 220, 2 );
    }

    tft.drawRoundRect( 155, 205, 105, 30, 6, TFT_RED );
    tft.drawString( "Back", 207, 220, 2 );
}

void drawCountrySelection() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "SELECT COUNTRY", 160, 30, 4 );

    tft.setTextDatum( ML_DATUM );

    int itemsPerScreen = 5;
    for ( int i = countryOffset; i < countryOffset + itemsPerScreen && i < COUNTRIES_COUNT; i++ ) {
        int idx = i - countryOffset;
        int yPos = 70 + idx * 30;
        String txt = String( countries[ i ].name );
        if ( txt.length() > 20 ) {
            txt = txt.substring( 0, 17 ) + "...";
        }
        tft.drawString( txt, 15, yPos, 2 );
        tft.drawFastHLine( 10, yPos + 20, 240, TFT_DARKGREY );
    }

    tft.drawString( "Custom lookup", 15, 70 + 5 * 30, 2 );

    if ( countryOffset > 0 ) {
        drawArrowUp( 265, 45, ( themeMode == THEME_BLUE ) ? yellowLight : TFT_BLUE );
    }
    if ( countryOffset + 5 < COUNTRIES_COUNT ) {
        drawArrowDown( 265, 180, ( themeMode == THEME_BLUE ) ? yellowLight : TFT_BLUE );
    }
    drawArrowBack( 265, 110, TFT_RED );
}

void drawCitySelection() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( selectedCountry, 160, 15, 2 );
    tft.drawString( "SELECT CITY", 160, 35, 4 );

    String cities[ 20 ];
    int cityCount = 0;
    getCountryCities( selectedCountry, cities, cityCount );

    tft.setTextDatum( ML_DATUM );

    int itemsPerScreen = 5;
    for ( int i = cityOffset; i < cityOffset + itemsPerScreen && i < cityCount; i++ ) {
        int idx = i - cityOffset;
        int yPos = 70 + idx * 30;
        String txt = cities[ i ];
        if ( txt.length() > 20 ) {
            txt = txt.substring( 0, 17 ) + "...";
        }
        tft.drawString( txt, 15, yPos, 2 );
        tft.drawFastHLine( 10, yPos + 20, 240, TFT_DARKGREY );
    }

    tft.drawString( "Custom lookup", 15, 70 + 5 * 30, 2 );

    if ( cityOffset > 0 ) {
        drawArrowUp( 265, 45, TFT_BLUE );
    }
    if ( cityOffset + 5 < cityCount ) {
        drawArrowDown( 265, 180, TFT_BLUE );
    }
    drawArrowBack( 265, 110, TFT_RED );
}

void drawLocationConfirm() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "CONFIRM LOCATION", 160, 30, 4 );

    tft.setTextDatum( ML_DATUM );
    tft.drawString( "City", 40, 80, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    tft.drawString( selectedCity, 40, 100, 2 );
    tft.setTextColor( getTextColor() );
    tft.drawString( "Country", 40, 130, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    tft.drawString( selectedCountry, 40, 150, 2 );
    tft.setTextColor( getTextColor() );
    tft.drawString( "Timezone", 40, 180, 2 );
    tft.setTextColor( TFT_SKYBLUE );
    tft.drawString( selectedTimezone, 40, 200, 2 );

    tft.setTextDatum( MC_DATUM );
    tft.drawRoundRect( 40, 205, 105, 30, 6, TFT_GREEN );
    tft.drawString( "SAVE", 92, 220, 2 );
    tft.drawRoundRect( 155, 205, 105, 30, 6, TFT_RED );
    tft.drawString( "Back", 207, 220, 2 );
}

void drawCustomCityInput() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.setFreeFont( &FreeSansBold12pt7b );
    tft.drawString( "Enter City Name", 160, 20 );

    // Input field - same design as WiFi keyboard
    tft.drawRect( 10, 40, 300, 30, isWhiteTheme ? TFT_BLACK : TFT_WHITE );
    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextDatum( ML_DATUM );
    tft.drawString( customCityInput, 20, 55 );

    // Keyboard - same design as WiFi
    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextDatum( MC_DATUM );

    const char *rows[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    if ( keyboardNumbers ) {
        rows[ 0 ] = "1234567890";
        rows[ 1 ] = "!@#$%^&*(/";
        rows[ 2 ] = ")-_+=.,?";
    }

    for ( int r = 0; r < 3; r++ ) {
        int len = strlen( rows[ r ] );
        for ( int i = 0; i < len; i++ ) {
            int btnX = i * 29 + 2;
            int btnY = 80 + r * 30;
            // Smaller squares, theme-appropriate colours (matching WiFi keyboard)
            tft.drawRect( btnX, btnY, 26, 26, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
            char ch = rows[ r ][ i ];
            if ( keyboardShift && !keyboardNumbers ) {
                ch = toupper( ch );
            }
            tft.drawString( String( ch ), btnX + 13, btnY + 15 );
        }
    }

    // Space bar
    tft.drawRect( 2, 170, 316, 25, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Space", 160, 183 );

    // Function buttons
    int bw = 64;
    int by = 198;
    int bh = 35;
    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );

    // 1. Shift/CAP
    tft.drawRect( 0 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "Shift", 0 * bw + bw / 2, by + 18 );

    // 2. 123
    tft.drawRect( 1 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "123", 1 * bw + bw / 2, by + 18 );

    // 3. Del
    tft.drawRect( 2 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "Del", 2 * bw + bw / 2, by + 18 );

    // 4. LOOKUP (Green - city-specific)
    tft.setTextColor( TFT_GREEN );
    tft.drawRect( 3 * bw + 2, by, bw - 4, bh, TFT_GREEN );
    tft.drawString( "SRCH", 3 * bw + bw / 2, by + 18 );

    // 5. BACK (Red/orange - city-specific)
    tft.setTextColor( TFT_ORANGE );
    tft.drawRect( 4 * bw + 2, by, bw - 4, bh, TFT_ORANGE );
    tft.drawString( "BACK", 4 * bw + bw / 2, by + 18 );

    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE ); // Reset colour
}

void drawFirmwareScreen() {
    tft.fillScreen( getBgColor() );

    if ( themeMode == THEME_BLUE ) {
        fillGradientVertical( 0, 0, 320, 240, blueDark, blueLight );
    }
    else if ( themeMode == THEME_YELLOW ) {
        fillGradientVertical( 0, 0, 320, 240, yellowDark, yellowLight );
    }

    // Nadpis
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "FIRMWARE", 160, 30, 4 );

    // Set datum for left column
    tft.setTextDatum( ML_DATUM );

    int yPos = 60;

    // Current version
    tft.setTextColor( getTextColor() );
    tft.drawString( "Current version:", 10, yPos, 2 );
    tft.setTextColor( TFT_GREEN );
    tft.drawString( String( FIRMWARE_VERSION ), 160, yPos, 2 );

    yPos += 25;

    // Available version
    tft.setTextColor( getTextColor() );
    tft.drawString( "Available:", 10, yPos, 2 );
    if ( availableVersion == "" || !updateAvailable ) {
        tft.setTextColor( TFT_DARKGREY );
        tft.drawString( "-", 160, yPos, 2 );
    }
    else {
        tft.setTextColor( TFT_ORANGE );
        tft.drawString( availableVersion, 160, yPos, 2 );
    }

    yPos += 35;

    // Install mode nadpis
    tft.setTextColor( getTextColor() );
    tft.drawString( "Install mode:", 10, yPos, 2 );

    yPos += 25;  // yPos is now 145

    // Radio buttons for install mode (Auto and By user only)
    const char *modes[ 2 ] = {"Auto", "By user"};
    for ( int i = 0; i < 2; i++ ) {
        int btnY = yPos + ( i * 25 ); // 145, 170

        // Radio button - circle centred on btnY
        tft.drawCircle( 20, btnY, 6, getTextColor() );
        if ( otaInstallMode == i ) {
            tft.fillCircle( 20, btnY, 4, TFT_GREEN );
        }

        // Text - correctly aligned with circle
        // ML_DATUM = Middle Left, so y is vertical centre of text
        // Circle is centred on btnY, text also centred on btnY
        tft.setTextColor( getTextColor() );
        tft.drawString( modes[ i ], 35, btnY, 2 );
    }

    // Reset text datum to centred for buttons
    tft.setTextDatum( MC_DATUM );

    // Check Now / Install button
    int btnY = 190;
    if ( updateAvailable ) {
        tft.fillRoundRect( 10, btnY, 140, 30, 5, TFT_GREEN );
        tft.setTextColor( TFT_BLACK );
        tft.drawString( "INSTALL", 80, btnY + 15, 2 );
    }
    else {
        tft.fillRoundRect( 10, btnY, 140, 30, 5, TFT_BLUE );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "CHECK NOW", 80, btnY + 15, 2 );
    }

    // Back button (same style as other menus)
    tft.drawRoundRect( 230, 125, 50, 50, 4, TFT_RED );
    drawArrowBack( 230, 125, TFT_RED );
}

void drawGraphicsScreen() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "GRAPHICS", 160, 30, 4 );

    // === THEMES ===
    tft.drawString( "Themes", 135, 50, 2 );

    // BLACK Theme
    tft.drawRoundRect( 20, 65, 50, 30, 4, TFT_BLACK );
    tft.drawRoundRect( 19, 64, 52, 32, 4, TFT_BLACK );
    tft.fillRoundRect( 21, 66, 48, 28, 3, themeMode == THEME_DARK ? TFT_WHITE : TFT_DARKGREY );
    tft.setTextColor( TFT_BLACK );
    tft.drawString( "BLK", 45, 78, 1 );
    tft.setTextColor( getTextColor() );

    // WHITE Theme
    tft.drawRoundRect( 80, 65, 50, 30, 4, TFT_WHITE );
    tft.drawRoundRect( 79, 64, 52, 32, 4, TFT_WHITE );
    tft.fillRoundRect( 81, 66, 48, 28, 3, themeMode == THEME_WHITE ? TFT_WHITE : TFT_DARKGREY );
    tft.setTextColor( TFT_BLACK );
    tft.drawString( "WHT", 105, 78, 1 );
    tft.setTextColor( getTextColor() );

    // BLUE Theme
    tft.drawRoundRect( 140, 65, 50, 30, 4, 0x0010 );
    tft.drawRoundRect( 139, 64, 52, 32, 4, 0x0010 );
    tft.fillRoundRect( 141, 66, 48, 28, 3, themeMode == THEME_BLUE ? 0x07FF : TFT_DARKGREY );
    tft.setTextColor( 0x0010 );
    tft.drawString( "BLU", 165, 78, 1 );
    tft.setTextColor( getTextColor() );

    // YELLOW Theme
    tft.drawRoundRect( 200, 65, 50, 30, 4, 0xCC00 );
    tft.drawRoundRect( 199, 64, 52, 32, 4, 0xCC00 );
    tft.fillRoundRect( 201, 66, 48, 28, 3, themeMode == THEME_YELLOW ? 0xFFE0 : TFT_DARKGREY );
    tft.setTextColor( TFT_BLACK );
    tft.drawString( "YEL", 225, 78, 1 );
    tft.setTextColor( getTextColor() );

    // === INVERT BUTTON ===
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Colours", 285, 50, 2 );

    // Invert button - same style as themes
    uint16_t invertBorderColor = invertColors ? TFT_GREEN : TFT_DARKGREY;
    uint16_t invertFillColor = invertColors ? TFT_GREEN : TFT_DARKGREY;

    tft.drawRoundRect( 260, 65, 50, 30, 4, invertBorderColor );
    tft.drawRoundRect( 259, 64, 52, 32, 4, invertBorderColor );
    tft.fillRoundRect( 261, 66, 48, 28, 3, invertFillColor );
    tft.setTextColor( TFT_BLACK );
    tft.drawString( "INV", 285, 78, 1 );
    tft.setTextColor( getTextColor() );

    // === DISPLAY BRIGHTNESS SLIDER (compact) ===
    tft.setTextDatum( ML_DATUM );
    tft.setTextColor( getTextColor() );
    tft.drawString( "Brightness", 10, 108, 2 );

    int sliderX = 10;
    int sliderY = 125;
    int sliderWidth = 130;
    int sliderHeight = 12;

    // Slider border
    tft.drawRect( sliderX, sliderY, sliderWidth, sliderHeight, getTextColor() );

    // Fill based on current brightness
    int fillWidth = map( brightness, 0, 255, 0, sliderWidth - 2 );
    tft.fillRect( sliderX + 1, sliderY + 1, fillWidth, sliderHeight - 2, TFT_SKYBLUE );

    // Percentage - placed immediately after compact slider
    int brightnessPercent = map( brightness, 0, 255, 0, 100 );
    tft.setTextDatum( ML_DATUM );
    tft.drawString( String( brightnessPercent ) + "%", sliderX + sliderWidth + 5, sliderY + 1, 1 );

    // === ANALOG / DIGITAL TOGGLE (to the right of brightness) ===
    int swX = 200;
    int swY = 115;
    int swW = 110;
    int swH = 28;

    // Active element colour
    uint16_t activeColor = TFT_GREEN;
    if ( themeMode == THEME_BLUE ) {
        activeColor = blueLight;
    }
    if ( themeMode == THEME_YELLOW ) {
        activeColor = yellowDark;
    }

    tft.drawRect( swX, swY, swW, swH, getTextColor() );

    if ( !isDigitalClock ) {
        // State: ANALOG (active left)
        tft.fillRect( swX + 2, swY + 2, ( swW / 2 ) - 2, swH - 4, activeColor );
        tft.setTextDatum( MC_DATUM );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "ANA", swX + ( swW / 4 ), swY + ( swH / 2 ), 2 );
        tft.setTextColor( getTextColor() );
        tft.drawString( "DIGI", swX + ( 3 * swW / 4 ), swY + ( swH / 2 ), 2 );
    }
    else {
        // State: DIGITAL (active right)
        tft.fillRect( swX + ( swW / 2 ), swY + 2, ( swW / 2 ) - 2, swH - 4, activeColor );
        tft.setTextDatum( MC_DATUM );
        tft.setTextColor( getTextColor() );
        tft.drawString( "ANA", swX + ( swW / 4 ), swY + ( swH / 2 ), 2 );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "DIGI", swX + ( 3 * swW / 4 ), swY + ( swH / 2 ), 2 );
    }

    // === ROTATE 180° TOGGLE ===
    int rotX = 200;
    int rotY = 148;
    int rotW = 110;
    int rotH = 22;

    tft.drawRect( rotX, rotY, rotW, rotH, getTextColor() );
    if ( !displayFlipped ) {
        tft.fillRect( rotX + 2, rotY + 2, ( rotW / 2 ) - 2, rotH - 4, activeColor );
        tft.setTextDatum( MC_DATUM );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "NRM", rotX + ( rotW / 4 ), rotY + ( rotH / 2 ), 1 );
        tft.setTextColor( getTextColor() );
        tft.drawString( "FLP", rotX + ( 3 * rotW / 4 ), rotY + ( rotH / 2 ), 1 );
    }
    else {
        tft.fillRect( rotX + ( rotW / 2 ), rotY + 2, ( rotW / 2 ) - 2, rotH - 4, activeColor );
        tft.setTextDatum( MC_DATUM );
        tft.setTextColor( getTextColor() );
        tft.drawString( "NRM", rotX + ( rotW / 4 ), rotY + ( rotH / 2 ), 1 );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "FLP", rotX + ( 3 * rotW / 4 ), rotY + ( rotH / 2 ), 1 );
    }

    // === AUTO DIM SETTINGS ===
    tft.setTextDatum( ML_DATUM );
    tft.setTextColor( getTextColor() );
    tft.drawString( "Auto Dim", 10, 155, 2 );

    // ON/OFF BUTTONS
    int onX = 10;
    int onY = 175;
    int offX = 10;
    int offY = 195;
    int btnWidth = 28;
    int btnHeight = 16;
    if ( autoDimEnabled ) {
        tft.fillRoundRect( onX, onY, btnWidth, btnHeight, 3, TFT_GREEN );
        tft.setTextColor( TFT_WHITE );
        tft.setTextDatum( MC_DATUM );
        tft.drawString( "ON", onX + btnWidth / 2, onY + btnHeight / 2, 1 );
        tft.fillRoundRect( offX, offY, btnWidth, btnHeight, 3, TFT_BLUE );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "OFF", offX + btnWidth / 2, offY + btnHeight / 2, 1 );
    }
    else {
        tft.fillRoundRect( onX, onY, btnWidth, btnHeight, 3, TFT_BLUE );
        tft.setTextColor( TFT_WHITE );
        tft.setTextDatum( MC_DATUM );
        tft.drawString( "ON", onX + btnWidth / 2, onY + btnHeight / 2, 1 );
        tft.fillRoundRect( offX, offY, btnWidth, btnHeight, 3, TFT_GREEN );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "OFF", offX + btnWidth / 2, offY + btnHeight / 2, 1 );
    }

    // SETTINGS TO THE RIGHT OF ON/OFF
    if ( autoDimEnabled ) {
        tft.setTextColor( getTextColor() );
        tft.setTextDatum( ML_DATUM );

        int startX = 50;
        int startY = onY + 3;
        int lineHeight = 16;
        // === START TIME ===
        tft.drawString( "Start", startX, startY, 1 );
        int startTimeX = startX + 50;
        tft.drawString( String( autoDimStart ) + "h", startTimeX, startY, 1 );
        int startPlusX = startTimeX + 50;
        int startPlusY = startY - 6;
        int btnW = 16;
        int btnH = 12;

        // Helper lambda: draw a +/- symbol centred in a rect using lines (font-metric-independent)
        auto drawPlusBtn = [ & ]( int rx, int ry ) {
            tft.drawRoundRect( rx, ry, btnW, btnH, 2, TFT_GREEN );
            int cx = rx + btnW / 2,  cy = ry + btnH / 2;
            tft.drawFastHLine( cx - 3, cy,     7, TFT_GREEN );
            tft.drawFastVLine( cx,     cy - 3, 7, TFT_GREEN );
        };
        auto drawMinusBtn = [ & ]( int rx, int ry ) {
            tft.drawRoundRect( rx, ry, btnW, btnH, 2, TFT_GREEN );
            int cx = rx + btnW / 2,  cy = ry + btnH / 2;
            tft.drawFastHLine( cx - 3, cy, 7, TFT_GREEN );
        };

        drawPlusBtn( startPlusX, startPlusY );
        int startMinusX = startPlusX + btnW + 10;
        int startMinusY = startPlusY;
        drawMinusBtn( startMinusX, startMinusY );
        tft.setTextColor( getTextColor() );

        // === END TIME ===
        int endY = startY + lineHeight;
        tft.setTextDatum( ML_DATUM );
        tft.drawString( "End", startX, endY, 1 );
        int endTimeX = startX + 50;
        tft.drawString( String( autoDimEnd ) + "h", endTimeX, endY, 1 );
        int endPlusX = endTimeX + 50;
        int endPlusY = endY - 6;
        drawPlusBtn( endPlusX, endPlusY );
        int endMinusX = endPlusX + btnW + 10;
        int endMinusY = endPlusY;
        drawMinusBtn( endMinusX, endMinusY );
        tft.setTextColor( getTextColor() );

        // === LEVEL - PROCENTA ===
        int levelY = endY + lineHeight;
        tft.setTextDatum( ML_DATUM );
        tft.drawString( "Level", startX, levelY, 1 );
        int levelTimeX = startX + 50;
        tft.drawString( String( autoDimLevel ) + "%", levelTimeX, levelY, 1 );
        int levelPlusX = levelTimeX + 50;
        int levelPlusY = levelY - 6;
        drawPlusBtn( levelPlusX, levelPlusY );
        int levelMinusX = levelPlusX + btnW + 10;
        int levelMinusY = levelPlusY;
        drawMinusBtn( levelMinusX, levelMinusY );
        tft.setTextColor( getTextColor() );
    }

    // === BACK BUTTON ===
    int backX = 252;
    int backY = 182;
    int backSize = 56;
    tft.drawRoundRect( backX, backY, backSize, backSize, 3, TFT_RED );
    tft.drawRoundRect( backX + 1, backY + 1, backSize - 2, backSize - 2, 2, TFT_RED );
    tft.setTextColor( TFT_RED );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "<", backX + backSize / 2, backY + backSize / 2, 4 );
}

// Partial repaint: only the slider fill and percentage label — no fillScreen flash.
void redrawBrightnessSlider() {
    const int sliderX      = 10;
    const int sliderY      = 125;
    const int sliderWidth  = 130;
    const int sliderHeight = 12;

    int fillWidth = map( brightness, 0, 255, 0, sliderWidth - 2 );
    // Filled portion
    if ( fillWidth > 0 ) {
        tft.fillRect( sliderX + 1, sliderY + 1, fillWidth, sliderHeight - 2, TFT_SKYBLUE );
    }
    // Empty portion (erase without touching the border)
    int emptyX = sliderX + 1 + fillWidth;
    int emptyW = sliderWidth - 2 - fillWidth;
    if ( emptyW > 0 ) {
        tft.fillRect( emptyX, sliderY + 1, emptyW, sliderHeight - 2, getBgColor() );
    }
    // Percentage label — draw with explicit background so the glyph cell self-clears (no artifact)
    int labelX = sliderX + sliderWidth + 5;
    int brightnessPercent = map( brightness, 0, 255, 0, 100 );
    tft.setTextDatum( ML_DATUM );
    tft.setTextColor( getTextColor(), getBgColor() );
    tft.drawString( String( brightnessPercent ) + "%  ", labelX, sliderY + 1, 1 );
    tft.setTextColor( getTextColor() );  // restore no-bg mode for other callers
}

void drawInitialSetup() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "WIFI SELECTION", 160, 15, 2 );
    tft.setTextDatum( ML_DATUM );

    if ( wifiCount == 0 ) {
        tft.drawString( "No networks found", 15, 45, 2 );
    }
    else {
        // Show up to 5 rows — row 5 is reserved for the pinned "Other..." entry
        for ( int i = wifiOffset; i < wifiOffset + 5 && i < wifiCount; i++ ) {
            int idx = i - wifiOffset;
            String txt = wifiSSIDs[ i ];
            if ( txt.length() > 18 ) {
                txt = txt.substring( 0, 15 ) + "...";
            }
            tft.drawString( txt, 15, 45 + idx * 30, 2 );
            tft.drawFastHLine( 10, 62 + idx * 30, 240, TFT_DARKGREY );
        }
    }

    // "Other..." is always pinned at row 5 — visible regardless of scroll position
    tft.setTextColor( TFT_BLUE );
    tft.drawString( "Other...", 15, 45 + 5 * 30, 2 );
    tft.drawFastHLine( 10, 62 + 5 * 30, 240, TFT_DARKGREY );
    tft.setTextColor( getTextColor() );

    // Navigation arrows
    // Back arrow - only if we already have a saved WiFi (not in initial setup with no data)
    if ( ssid != "" ) {
        drawArrowBack( 265, 50, TFT_RED );
    }

    // Up arrow
    if ( wifiOffset > 0 ) {
        drawArrowUp( 265, 110, TFT_BLUE );
    }

    // Down arrow — more networks exist above the pinned "Other..." row
    if ( wifiOffset + 5 < wifiCount ) {
        drawArrowDown( 265, 170, TFT_BLUE );
    }
}

void drawKeyboardScreen() {
    tft.fillScreen( getBgColor() );
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.setFreeFont( &FreeSansBold12pt7b );

    String title = "WiFi Password";
    if ( currentState == SSID_INPUT ) {
        title = "Enter SSID";
    }
    else if ( currentState == CUSTOMCITYINPUT ) {
        title = "Enter City Name";
    }
    else if ( currentState == CUSTOMCOUNTRYINPUT ) {
        title = "Enter Country Name";
    }
    tft.drawString( title, 160, 20 );

    tft.drawRect( 10, 40, 300, 30, isWhiteTheme ? TFT_BLACK : TFT_WHITE );
    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextDatum( ML_DATUM );
    // DISPLAY LOGIC: Asterisks only for WiFi password and only when showPassword is false
    if ( currentState == KEYBOARD && !showPassword ) {
        String stars = "";
        for ( int i = 0; i < passwordBuffer.length(); i++ ) {
            stars += "*";
        }
        tft.drawString( stars, 20, 55 );
    }
    else {
        tft.drawString( passwordBuffer, 20, 55 );
    }

    // VISIBILITY TOGGLE (only for WiFi password keyboard, not SSID entry)
    if ( currentState == KEYBOARD ) {
        tft.setTextDatum( MC_DATUM );
        tft.drawRect( 250, 140, 60, 25, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
        tft.setFreeFont( &FreeSans9pt7b );
        tft.drawString( showPassword ? "Hide" : "Show", 280, 153 );
    }

    // Keyboard
    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextDatum( MC_DATUM );

    const char *rows[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    if ( keyboardNumbers ) {
        // NUMBER MODE: row 1 digits, rows 2 and 3 special characters
        rows[ 0 ] = "1234567890";
        rows[ 1 ] = "!@#$%^&*(/";
        rows[ 2 ] = ")-_+=.,?";
    }

    for ( int r = 0; r < 3; r++ ) {
        int len = strlen( rows[ r ] );
        for ( int i = 0; i < len; i++ ) {
            int btnX = i * 29 + 2;
            int btnY = 80 + r * 30;
            tft.drawRect( btnX, btnY, 26, 26, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
            char ch = rows[ r ][ i ];
            if ( keyboardShift && !keyboardNumbers ) {
                ch = toupper( ch );
            }
            tft.drawString( String( ch ), btnX + 13, btnY + 15 );
        }
    }

    // Space bar and function buttons
    tft.drawRect( 2, 170, 316, 25, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "Space", 160, 183 );

    int bw = 64;
    int by = 198;
    int bh = 35;
    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );
    tft.drawRect( 0 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "Shift", 0 * bw + bw / 2, by + 18 );
    tft.drawRect( 1 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "123", 1 * bw + bw / 2, by + 18 );
    tft.drawRect( 2 * bw + 2, by, bw - 4, bh, isWhiteTheme ? TFT_DARKGREY : TFT_WHITE );
    tft.drawString( "Del", 2 * bw + bw / 2, by + 18 );
    tft.setTextColor( TFT_RED );
    tft.drawRect( 3 * bw + 2, by, bw - 4, bh, TFT_RED );
    tft.drawString( "Back", 3 * bw + bw / 2, by + 18 );
    tft.setTextColor( TFT_GREEN );
    tft.drawRect( 4 * bw + 2, by, bw - 4, bh, TFT_GREEN );
    tft.drawString( "OK", 4 * bw + bw / 2, by + 18 );
    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );
}

void updateKeyboardText() {
    // Clear only the inside of the text box to avoid flickering the rest of the keyboard
    tft.fillRect( 11, 41, 298, 28, isWhiteTheme ? TFT_WHITE : TFT_BLACK );

    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextColor( isWhiteTheme ? TFT_BLACK : TFT_WHITE );
    tft.setTextDatum( ML_DATUM );

    if ( currentState == KEYBOARD && !showPassword ) {
        String stars = "";
        for ( int i = 0; i < passwordBuffer.length(); i++ ) {
            stars += "*";
        }
        tft.drawString( stars, 20, 55 );
    }
    else {
        tft.drawString( passwordBuffer, 20, 55 );
    }
}

void handleKeyboardTouch( int x, int y ) {
    log_d( "[KEYBOARD] Touch detected at X=%d, Y=%d", x, y );
    // ========== DETECT LETTERS AND DIGITS IN KEYBOARD ROWS ==========
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

            // Touch area adjusted to 26x26 (matches WiFi keyboard design)
            if ( x >= btnX && x <= btnX + 26 && y >= btnY && y <= btnY + 26 ) {
                char ch = row[ i ];
                if ( keyboardShift && !keyboardNumbers ) {
                    ch = toupper( ch );
                }

                if ( currentState == KEYBOARD ) {
                    passwordBuffer += ch;
                    log_d( "[KEYBOARD] Added to passwordBuffer: %c", ch );
                    updateKeyboardText();
                }
                else if ( currentState == CUSTOMCITYINPUT ) {
                    customCityInput += ch;
                    log_d( "[KEYBOARD] Added to customCityInput: %c", ch );
                    drawCustomCityInput(); // Full redraw to keep visual consistent
                }
                else if ( currentState == CUSTOMCOUNTRYINPUT ) {
                    customCountryInput += ch;
                    log_d( "[KEYBOARD] Added to customCountryInput: %c", ch );
                    drawCustomCountryInput(); // Full redraw to keep visual consistent
                }

                delay( UI_DEBOUNCE_MS );
                return;
            }
        }
    }

    // ========== DETECT SPACE BAR ==========
    if ( x >= 2 && x <= 318 && y >= 170 && y <= 195 ) {
        log_d( "[KEYBOARD] Space pressed" );
        if ( currentState == KEYBOARD ) {
            passwordBuffer += " ";
            updateKeyboardText();
        }
        else if ( currentState == CUSTOMCITYINPUT ) {
            customCityInput += " ";
            drawCustomCityInput();
        }
        else if ( currentState == CUSTOMCOUNTRYINPUT ) {
            customCountryInput += " ";
            drawCustomCountryInput();
        }

        delay( UI_DEBOUNCE_MS );
        return;
    }

    // ========== DETECT FUNCTION BUTTONS ==========
    int bw = 64;
    int by = 198;
    int bh = 35;

    // ===== BUTTON 1: SHIFT (CAP) =====
    if ( x >= 0 && x <= bw && y >= by && y <= by + bh ) {
        log_d( "[KEYBOARD] SHIFT pressed" );
        keyboardShift = !keyboardShift;

        if ( currentState == KEYBOARD ) {
            drawKeyboardScreen();
        }
        else if ( currentState == CUSTOMCITYINPUT ) {
            drawCustomCityInput();
        }
        else if ( currentState == CUSTOMCOUNTRYINPUT ) {
            drawCustomCountryInput();
        }

        delay( UI_DEBOUNCE_MS );
        return;
    }

    // ===== BUTTON 2: NUMBERS (123) =====
    if ( x >= bw && x <= 2 * bw && y >= by && y <= by + bh ) {
        log_d( "[KEYBOARD] NUMBERS toggle pressed" );
        keyboardNumbers = !keyboardNumbers;

        if ( currentState == KEYBOARD ) {
            drawKeyboardScreen();
        }
        else if ( currentState == CUSTOMCITYINPUT ) {
            drawCustomCityInput();
        }
        else if ( currentState == CUSTOMCOUNTRYINPUT ) {
            drawCustomCountryInput();
        }

        delay( UI_DEBOUNCE_MS );
        return;
    }

    // ===== BUTTON 3: DELETE (DEL) =====
    if ( x >= 2 * bw && x <= 3 * bw && y >= by && y <= by + bh ) {
        log_d( "[KEYBOARD] DEL pressed" );
        if ( currentState == KEYBOARD ) {
            if ( passwordBuffer.length() > 0 ) {
                passwordBuffer.remove( passwordBuffer.length() - 1 );
                updateKeyboardText();
            }
        }
        else if ( currentState == CUSTOMCITYINPUT ) {
            if ( customCityInput.length() > 0 ) {
                customCityInput.remove( customCityInput.length() - 1 );
                drawCustomCityInput();
            }
        }
        else if ( currentState == CUSTOMCOUNTRYINPUT ) {
            if ( customCountryInput.length() > 0 ) {
                customCountryInput.remove( customCountryInput.length() - 1 );
                drawCustomCountryInput();
            }
        }

        delay( UI_DEBOUNCE_MS );
        return;
    }

    // ===== BUTTON 4: LOOKUP / SEARCH / OK (WiFi) =====
    if ( x >= 3 * bw && x <= 4 * bw && y >= by && y <= by + bh ) {
        // 1) WiFi - BACK button (in WiFi keyboard design: 3*bw = Back(Red), 4*bw = OK(Green))
        // In drawCustomCityInput: 3*bw = Lookup(Green), 4*bw = Back(Orange)

        // Must distinguish by currentState which button is at which position

        if ( currentState == KEYBOARD ) {
            // For WiFi, position 3*bw is the BACK button
            log_d( "[KEYBOARD] WIFI BACK pressed" );
            passwordBuffer = ""; // CLEAR TEXT ON EXIT
            keyboardShift = false;
            keyboardNumbers = false;
            currentState = WIFICONFIG;
            drawInitialSetup();

        }
        else if ( currentState == CUSTOMCITYINPUT ) {
            // For City Input, position 3*bw is the LOOKUP button (Green)
            log_d( "[KEYBOARD] LOOKUP pressed for city" );
            if ( customCityInput.length() > 0 ) {
                log_d( "[KEYBOARD] Looking up city: %s", customCityInput.c_str() );
                lookupCityGeonames( customCityInput, selectedCountry );
                currentState = CITYLOOKUPCONFIRM;
                drawCityLookupConfirm();
            }
            else {
                log_d( "[KEYBOARD] City input empty, cannot lookup" );
                delay( UI_SPLASH_DELAY_MS );
                drawCustomCityInput();
            }
        }
        else if ( currentState == CUSTOMCOUNTRYINPUT ) {
            // For Country Input, position 3*bw is the SEARCH button (Green)
            log_d( "[KEYBOARD] SEARCH pressed for country" );
            if ( customCountryInput.length() > 0 ) {
                log_d( "[KEYBOARD] Looking up country: %s", customCountryInput.c_str() );
                lookupCountryGeonames( customCountryInput );
                currentState = COUNTRYLOOKUPCONFIRM;
                drawCountryLookupConfirm();
            }
            else {
                log_d( "[KEYBOARD] Country input empty, cannot lookup" );
                delay( UI_SPLASH_DELAY_MS );
                drawCustomCountryInput();
            }
        }

        delay( UI_DEBOUNCE_MS );
        return;
    }

    // ===== BUTTON 5: BACK (Custom) / OK (WiFi) =====
    if ( x >= 4 * bw && x <= 5 * bw && y >= by && y <= by + bh ) {

        if ( currentState == KEYBOARD ) {
            // For WiFi, position 4*bw is the OK button (Green)
            log_d( "[KEYBOARD] WIFI OK pressed" );
            prefs.begin( "sys", false );
            prefs.putString( "ssid", selectedSSID );
            prefs.putString( "pass", obfuscatePassword( passwordBuffer ) );
            prefs.end();

            ssid = selectedSSID;
            password = passwordBuffer;

            showWifiConnectingScreen( ssid );
            WiFi.mode( WIFI_STA );
            WiFi.scanDelete();
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

        }
        else if ( currentState == CUSTOMCITYINPUT ) {
            // For City Input, position 4*bw is the BACK button
            log_d( "[KEYBOARD] Returning from custom city input" );
            customCityInput = ""; // CLEAR TEXT ON EXIT
            keyboardShift = false;
            keyboardNumbers = false;
            currentState = CITYSELECT;
            cityOffset = 0;
            drawCitySelection();

        }
        else if ( currentState == CUSTOMCOUNTRYINPUT ) {
            // For Country Input, position 4*bw is the BACK button
            log_d( "[KEYBOARD] Returning from custom country input" );
            customCountryInput = ""; // CLEAR TEXT ON EXIT
            keyboardShift = false;
            keyboardNumbers = false;
            currentState = COUNTRYSELECT;
            countryOffset = 0;
            drawCountrySelection();
        }

        delay( UI_DEBOUNCE_MS );
        return;
    }

    // ===== SHOW/HIDE PASSWORD (WIFI ONLY) =====
    if ( currentState == KEYBOARD && x >= 250 && x <= 320 && y >= 140 && y <= 165 ) {
        log_d( "[KEYBOARD] SHOW/HIDE password toggled" );
        showPassword = !showPassword;
        drawKeyboardScreen();
        delay( UI_DEBOUNCE_MS );
        return;
    }
}

