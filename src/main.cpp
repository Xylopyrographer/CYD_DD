/*

This is DataDisplayCYD.ino, with some additions.
Ref: https://github.com/lachimalaif/DataDisplay-V1-instalator/tree/main

*/

// startup issue solved (connection successful and no more)
// possibility of hand correction of Timezone in manual mode of regional setup
// posix from api
// right time over all world
// hPa to inHg possibiliity
// manual coordinates setup possibility

// --- System / Arduino libraries ---
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Update.h>
#include <WiFi.h>
#include <XPT2046_Touchscreen.h>
#include "time.h"

// --- Application modules ---
#include "app/location.h"
#include "data/app_state.h"
#include "data/city_data.h"
#include "data/nameday.h"
#include "data/recent.h"
#include "hal/backlight.h"
#include "hal/led.h"
#include "net/location.h"
#include "net/ota.h"
#include "net/timezone.h"
#include "net/holidays.h"
#include "net/weather_api.h"
#include "ui/clock_face.h"
#include "ui/icons.h"
#include "ui/screens.h"
#include "ui/theme.h"
#include "ui/touch_handler.h"
#include "util/constants.h"
#include "util/credentials.h"
#include "util/moon.h"
#include "util/string_utils.h"

// ================= TOUCHSCREEN PIN DEFINITIONS =================
#define T_CS 33
#define T_IRQ 36
#define T_CLK 25
#define T_DIN 32
#define T_DOUT 39

// ================= GLOBAL SETTINGS (Must be FIRST) =================
TFT_eSPI tft = TFT_eSPI();  // pins defined in: include/User_Setup.h
XPT2046_Touchscreen ts( T_CS, T_IRQ );
Preferences prefs;
bool isWhiteTheme = false;  // NOW IT'S HERE, SO EVERYONE CAN SEE IT

// ================= NEW VARIABLES FOR CLOCKS =================
bool isDigitalClock = false; // false = Analog, true = Digital
bool is12hFormat = false;    // false = 24h, true = 12h
bool invertColors = false;  // NEW VARIABLE: Invert colors for CYD boards with inverted displays
bool displayFlipped = false; // true = rotation 3 (180° flipped), false = rotation 1 (normal)

// ================= OTA UPDATE GLOBALS =================
const char *FIRMWARE_VERSION = "1.4.1";  // CURRENT VERSION
const char *VERSION_CHECK_URL = "https://raw.githubusercontent.com/lachimalaif/DataDisplay-V1-instalator/main/version.json";
const char *FIRMWARE_URL = "https://github.com/lachimalaif/DataDisplay-V1-instalator/releases/latest/download/DataDisplayCYD.ino.bin";

String availableVersion = "";   // Available version from GitHub
String downloadURL = "";        // Firmware download URL (from version.json)
bool updateAvailable = false;   // Is there an update available?
int otaInstallMode = 1;         // 0=Auto, 1=By user, 2=Manual
unsigned long lastVersionCheck = 0;
const unsigned long VERSION_CHECK_INTERVAL = 86400000;  // 24 hours (for testing change to 30000 = 30s)

bool isUpdating = false;  // Is the update in progress?
int updateProgress = 0;   // Progress 0-100%
String updateStatus = ""; // Status message

// ================= TEMA NASTAVENI =================
int themeMode = THEME_DARK; // THEME_DARK=0, THEME_WHITE=1, THEME_BLUE=2, THEME_YELLOW=3
// NOTE: For BLACK and WHITE themes, isWhiteTheme specifies: false=BLACK, true=WHITE
// For BLUE and YELLOW themes, isWhiteTheme is ignored (solid colors)

float themeTransition = 0.0f; // Transition progress(0.0 - 1.0)

// Gradient colours
uint16_t blueLight = 0x07FF;    // Light blue
uint16_t blueDark = 0x0010;     // Dark blue
uint16_t yellowLight = 0xFFE0;  // Light yellow
uint16_t yellowDark = 0xCC00;   // Dark yellow

// ================= WEATHER GLOBALS =================
String weatherCity = "Plzen";
float currentTemp = 0.0;
int currentHumidity = 0;
float currentWindSpeed = 0.0;
int currentWindDirection = 0;
int currentPressure = 0;
int weatherCode = 0;
float lat = 0;
float lon = 0;
bool weatherUnitF = false;
bool weatherUnitMph = false;  // false = km/h, true = mph
bool weatherUnitInHg = false; // false = hPa, true = inHg
float lookupLat = 0.0;
float lookupLon = 0.0;
String coordLatBuffer = "";
String coordLonBuffer = "";
bool coordEditingLon = false;
unsigned long lastWeatherUpdate = 0;
bool initialWeatherFetched = false;

ForecastData forecast[ 2 ];
// Forecast day name variables
String forecastDay1Name = "Monday";    // Tomorrow
String forecastDay2Name = "Tuesday";   // The day after tomorrow

int moonPhaseVal = 0;

// ================= SUN AND AUTO DIM (NEW FROM control.txt) =================
String sunriseTime = "--:--";
String sunsetTime = "--:--";

// ================= AUTODIM UI - SETTINGS IN THE MENU =================
int autoDimEditMode = 0;  // 0=none, 1=editing start, 2=editing end, 3=editing level
int autoDimTempStart = 22;
int autoDimTempEnd = 6;
int autoDimTempLevel = 20;
unsigned long lastBrightnessUpdate = 0;  // So that the brightness does not change with each loop

bool autoDimEnabled = false;
int autoDimStart = 22;
int autoDimEnd = 6;
int autoDimLevel = 20;
bool isDimmed = false;

// Sun icons
extern const unsigned char icon_sunrise[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x01, 0x80, 0x03, 0xc0, 0x05, 0xa0,
    0x09, 0x90, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
extern const unsigned char icon_sunset[] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x09, 0x90, 0x05, 0xa0, 0x03, 0xc0,
    0x01, 0x80, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const char *ntpServer = "pool.ntp.org";
long gmtOffset_sec = 3600;
int daylightOffset_sec = 0;

extern const int clockX = 230;
extern const int clockY = 85;
extern const int radius = 67;
int lastHour = -1, lastMin = -1, lastSec = -1, lastDay = -1;
int brightness = 255;
String cityName = "Plzen";
unsigned long lastWifiStatusCheck = 0;
int lastWifiStatus = -1;
bool forceClockRedraw = false;

// ScreenState, ForecastData, RecentCity, MAX_RECENT_CITIES → src/data/app_state.h
ScreenState currentState = CLOCK;

bool regionAutoMode = true;
String selectedCountry = "";
String selectedCity;
String selectedTimezone;
String customCityInput;
String customCountryInput;
String lookupCountry;
String lookupCity;
String lookupTimezone;
String countryName = "Czech Republic";
String timezoneName = "Europe/Prague";
int lookupGmtOffset = 3600;
int lookupDstOffset = 3600;
String posixTZ = "CET-1CEST,M3.5.0,M10.5.0/3";

RecentCity recentCities[ MAX_RECENT_CITIES ];
int recentCount = 0;

unsigned long lastTouchTime = 0;
int menuOffset = 0;
int countryOffset = 0;
int cityOffset = 0;
extern const int MENU_BASE_Y = 70;
extern const int MENU_ITEM_HEIGHT = 35;
extern const int MENU_ITEM_GAP = 8;
extern const int MENU_ITEM_SPACING = MENU_ITEM_HEIGHT + MENU_ITEM_GAP;

String ssid, password, selectedSSID, passwordBuffer;
extern const int MAX_NETWORKS = 20;
String wifiSSIDs[ MAX_NETWORKS ];
int wifiCount = 0, wifiOffset = 0;
bool keyboardNumbers = false;
bool keyboardShift = false;
bool showPassword = false; // Default: password is hidden (asterisks)

int touchXMin = 200;    // Normal orientation cal — overridden from NVS if saved
int touchXMax = 3900;
int touchYMin = 200;
int touchYMax = 3900;
int touchXMinF = 3900;  // Flipped orientation cal defaults (axes reversed relative to normal)
int touchXMaxF = 200;
int touchYMinF = 3900;
int touchYMaxF = 200;
const int SCREEN_WIDTH  = 320;
const int SCREEN_HEIGHT = 240;
constexpr float DEGTORAD = ( float )( PI / 180.0 ); // Degrees to radians conversion

void setup() {
    // Kill backlight FIRST — before tft.init()
    //   LEDC will take over this pin shortly.
    pinMode( TFT_BL, OUTPUT );
    digitalWrite( TFT_BL, LOW );

    Serial.begin( 115200 );
    delay( 500 );
    initLEDS();

    log_i( "[SETUP] === CYD Starting ===" );
    log_i( "[SETUP] Version: %s", FIRMWARE_VERSION );

    // ===== PREFERENCES INITIALIZATION (Load settings) =====
    // Must load preferences BEFORE initialising TFT so we know the background colour
    // Use isKey() to detect a fresh/erased device without triggering NVS NOT_FOUND log errors.
    // Open read-write (not read-only) so the namespace is created if absent — avoids the
    // nvs_open NOT_FOUND error that fires when the namespace has never been written.
    prefs.begin( "sys", false );  // creates namespace if absent; isKey() still false on fresh device
    bool nvsInitialized = prefs.isKey( "ssid" );
    prefs.end();

    if ( nvsInitialized ) {
        prefs.begin( "sys", false );
        ssid = prefs.getString( "ssid", "" );
        password = deobfuscatePassword( prefs.getString( "pass", "" ) );
        isDigitalClock = prefs.getBool( "digiClock", false );
        is12hFormat = prefs.getBool( "12hFmt", false );

        // FIX: Load saved theme
        themeMode = prefs.getInt( "themeMode", THEME_DARK );
        isWhiteTheme = prefs.getBool( "theme", false );
        invertColors = prefs.getBool( "invertColors", false );
        displayFlipped = prefs.getBool( "dispFlip", false );

        // Load OTA settings
        otaInstallMode = prefs.getInt( "otaMode", 1 ); // Default: By user
        log_d( "[OTA] Install mode: %d", otaInstallMode );

        // FIX: Load brightness and Auto Dim settings
        brightness = prefs.getInt( "bright", 255 ); // Load saved brightness
        autoDimEnabled = prefs.getBool( "autoDimEnabled", false );
        autoDimStart = prefs.getInt( "autoDimStart", 22 );
        autoDimEnd = prefs.getInt( "autoDimEnd", 6 );
        autoDimLevel = prefs.getInt( "autoDimLevel", 20 );

        // Load touch calibration for the active orientation
        if ( displayFlipped ) {
            touchXMin = prefs.getInt( "calXMinF", 3900 );
            touchXMax = prefs.getInt( "calXMaxF",  200 );
            touchYMin = prefs.getInt( "calYMinF", 3900 );
            touchYMax = prefs.getInt( "calYMaxF",  200 );
        }
        else {
            touchXMin = prefs.getInt( "calXMin",  200 );
            touchXMax = prefs.getInt( "calXMax", 3900 );
            touchYMin = prefs.getInt( "calYMin",  200 );
            touchYMax = prefs.getInt( "calYMax", 3900 );
        }

        // FIX: Load temperature unit setting (°C / °F)
        weatherUnitF = prefs.getBool( "weatherUnitF", false );
        weatherUnitMph = prefs.getBool( "weatherUnitMph", false );
        weatherUnitInHg = prefs.getBool( "weatherUnitInHg", false );
        log_d( "[SETUP] Weather unit loaded: %s", weatherUnitF ? "°F" : "°C" );

        prefs.end();
        log_d( "[SETUP] Preferences loaded - Theme: %d, AutoDim: %d, InvertColors: %s", themeMode, autoDimEnabled, invertColors ? "TRUE" : "FALSE" );
    } // end if ( nvsInitialized )

    // ===== TFT LCD INITIALIZATION =====
    tft.init();
    tft.setRotation( displayFlipped ? 3 : 1 );

    // NOTE: ILI9341 (original CYD) does NOT have a hardware-inverted display.
    // invertColors=false → normal display, invertColors=true → inverted display.
    // (The CYD2U/ST7789 version used !invertColors to compensate HW inversion.)
    delay( 50 );
    tft.invertDisplay( invertColors );
    delay( 50 );

    tft.fillScreen( getBgColor() ); // Fill screen with theme colour while backlight is still off
    backlightInit( brightness );     // Attach LEDC and reveal the screen at user brightness

    log_d( "[SETUP] Display inverted (SW): %s | User wants inversion: %s", !invertColors ? "TRUE" : "FALSE", invertColors ? "YES" : "NO" );

    log_d( "[SETUP] TFT initialized" );

    // ===== TOUCHSCREEN INITIALIZATION =====
    SPI.begin( T_CLK, T_DOUT, T_DIN );
    ts.begin();
    ts.setRotation( 1 ); // Always 1 — coordinate mirroring for flip is handled by the loop-level map() min/max swap

    log_d( "[SETUP] Touchscreen initialized" );

    // ===== UI INITIALIZATION =====
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );

    // ===== LOAD SAVED LOCATION =====
    if ( nvsInitialized ) {
        loadSavedLocation();
        loadRecentCities();
    }
    weatherCity = cityName;

    log_i( "[SETUP] Location loaded: %s", cityName.c_str() );

    // ===== NAMEDAY VARIABLES =====
    lastNamedayDay = -1;
    lastNamedayHour = -1;

    // ===== WIFI CONNECTION IF SAVED =====
    if ( ssid != "" ) {
        log_d( "[SETUP] Attempting WiFi connection with saved SSID: %s", ssid.c_str() );
        showWifiConnectingScreen( ssid );

        WiFi.mode( WIFI_STA );
        WiFi.begin( ssid.c_str(), password.c_str() );

        unsigned long start = millis();
        while ( WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT ) {
            delay( 500 );
        }

        if ( WiFi.status() == WL_CONNECTED ) {
            log_i( "[SETUP] WiFi connected successfully" );
            showWifiResultScreen( true );

            if ( regionAutoMode ) {
                log_d( "[SETUP] Auto-sync enabled, syncing region..." );
                syncRegion();
            }

            // FIX: Ensure NTP sync with active WiFi regardless of syncRegion() result.
            // If syncRegion fails, applyLocation() is never called and configTime() never
            // runs with an active network — SNTP daemon waits up to 15 min for retry.
            // This call restarts NTP sync using posixTZ loaded from preferences.
            log_d( "[SETUP] Re-applying NTP config with active WiFi..." );
            configTime( 0, 0, ntpServer );
            setenv( "TZ", posixTZ.c_str(), 1 );
            tzset();

            currentState = CLOCK;
            lastSec = -1;  // Force full redraw in loop()

            handleNamedayUpdate();
            handleHolidayUpdate();
        }
        else {
            log_w( "[SETUP] WiFi connection failed" );
            showWifiResultScreen( false );
            currentState = WIFICONFIG;
            scanWifiNetworks();
            drawInitialSetup();
        }
    }
    else {
        log_i( "[SETUP] No saved WiFi, showing setup screen" );
        currentState = WIFICONFIG;
        scanWifiNetworks();
        drawInitialSetup();
    }

    log_i( "[SETUP] === Setup complete ===" );
}

// getNamedayForDate() and handleNamedayUpdate() moved to src/data/nameday.cpp
// fetchTodayHoliday() and handleHolidayUpdate() moved to src/net/holidays.cpp


void loop() {
    // AUTODIM LOGIC
    if ( millis() - lastBrightnessUpdate > BRIGHTNESS_UPDATE_INTERVAL ) {
        applyAutoDim();
        lastBrightnessUpdate = millis();
    }

    // 1. WiFi CONNECTION CHECK
    if ( WiFi.status() != WL_CONNECTED ) {
        if ( currentState != WIFICONFIG && currentState != KEYBOARD && currentState != SSID_INPUT && currentState != CUSTOMCITYINPUT && currentState != CUSTOMCOUNTRYINPUT &&
                currentState != SETTINGS && currentState != WEATHERCONFIG && currentState != REGIONALCONFIG && currentState != GRAPHICSCONFIG &&
                currentState != FIRMWARE_SETTINGS && currentState != COUNTRYSELECT && currentState != CITYSELECT && currentState != LOCATIONCONFIRM &&
                currentState != COUNTRYLOOKUPCONFIRM && currentState != CITYLOOKUPCONFIRM ) {
            currentState = CLOCK;
        }

        static unsigned long lastReconnectAttempt = 0;
        if ( millis() - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL ) {
            log_i( "WIFI: Attempting reconnect..." );
            WiFi.reconnect();
            lastReconnectAttempt = millis();
        }
    }

    // 2. TOUCH HANDLING
    if ( ts.touched() ) {
        if ( millis() - lastTouchTime < TOUCH_DEBOUNCE_MS ) {
            return;
        }
        lastTouchTime = millis();

        TS_Point p = ts.getPoint();
        // Cal values already encode orientation (flipped set has min/max reversed)
        int x = map( p.x, touchXMin, touchXMax, 0, SCREEN_WIDTH  );
        int y = map( p.y, touchYMin, touchYMax, 0, SCREEN_HEIGHT );
        x = constrain( x, 0, SCREEN_WIDTH - 1 );
        y = constrain( y, 0, SCREEN_HEIGHT - 1 );

        handleTouch( x, y );
    }

    // 3. CLOCK AND WEATHER LOGIC
    if ( currentState == CLOCK ) {
        struct tm ti;
        if ( getLocalTime( &ti ) ) {
            if ( ti.tm_sec != lastSec ) {
                static bool initialRender = true;
                if ( lastSec == -1 ) {
                    // On the very first clock draw, keep the backlight off while
                    // we fetch weather and build the full layout so nothing
                    // partially-formed is ever visible.
                    if ( initialRender ) {
                        backlightSet( 0 );
                    }

                    tft.fillScreen( getBgColor() );
                    if ( themeMode == THEME_BLUE ) {
                        fillGradientVertical( 0, 0, 320, 240, blueDark, blueLight );
                    }
                    else if ( themeMode == THEME_YELLOW ) {
                        fillGradientVertical( 0, 0, 320, 240, yellowDark, yellowLight );
                    }

                    forceClockRedraw = true;
                    handleNamedayUpdate();
                    handleHolidayUpdate();

                    // drawClockFace() / drawClockStatic() removed: the sprite in
                    // updateHands() redraws border + ticks + numbers + hands atomically,
                    // so drawing to TFT here is redundant and would erase the gradient.

                    if ( lastWeatherUpdate == 0 && cityName != "" ) {
                        weatherCity = cityName;
                        fetchWeatherData();
                        lastWeatherUpdate = millis();
                    }

                    drawWeatherSection();
                    drawDateAndWeek( &ti );
                    drawSettingsIcon( TFT_SKYBLUE );
                    drawWifiIndicator();
                    drawUpdateIndicator();
                }

                updateHands( ti.tm_hour, ti.tm_min, ti.tm_sec );
                if ( initialRender ) {
                    backlightSet( brightness );   // Reveal the fully-formed display
                    initialRender = false;
                }
                lastHour = ti.tm_hour;
                lastMin  = ti.tm_min;
                lastSec  = ti.tm_sec;
            }

            // Leave the rest of the day-change handler unchanged.
            if ( ti.tm_mday != lastDay ) {
                lastDay = ti.tm_mday;
                handleNamedayUpdate();
                handleHolidayUpdate();
                drawDateAndWeek( &ti );
                drawSettingsIcon( TFT_SKYBLUE );
                drawWifiIndicator();
                drawUpdateIndicator();
            }
        }

        if ( millis() - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL ) {
            if ( WiFi.status() == WL_CONNECTED && cityName != "" ) {
                fetchWeatherData();
                drawWeatherSection();
                lastWeatherUpdate = millis();
            }
        }
    }
    // OTA version check (at startup and every X hours)
    if ( !isUpdating && WiFi.status() == WL_CONNECTED ) {
        if ( lastVersionCheck == 0 || ( millis() - lastVersionCheck > VERSION_CHECK_INTERVAL ) ) {
            checkForUpdate();

            // Debug: Display what we loaded
            if ( updateAvailable ) {
                log_i( "[OTA] Update check complete: v%s url=%s", availableVersion.c_str(), downloadURL.c_str() );
            }

            // If an update is available, force icon redraw
            if ( updateAvailable && currentState == CLOCK ) {
                drawUpdateIndicator();  // Show icon immediately
            }

            // If an update is available and mode is AUTO
            if ( updateAvailable && otaInstallMode == 0 ) {
                log_i( "[OTA] Auto-update mode - starting update..." );
                performOTAUpdate();
            }
        }
    }
    delay( 20 );
}


//  --- EOF --- //
