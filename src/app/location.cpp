#include "location.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "../util/constants.h"
#include "../data/app_state.h"
#include "../data/nameday.h"
#include "../net/location.h"
#include "../net/timezone.h"
#include "../net/holidays.h"

// Externs defined in main.cpp
extern ScreenState currentState;
extern Preferences prefs;
extern const char *ntpServer;

// Location globals
extern String selectedCity;
extern String selectedCountry;
extern String selectedTimezone;
extern String posixTZ;
extern String cityName;
extern String countryName;
extern float  lat;
extern float  lon;
extern float  lookupLat;
extern float  lookupLon;
extern String lookupCountry;
extern String lookupISOCode;
extern String lookupCity;
extern String lookupTimezone;
extern int    lookupGmtOffset;
extern int    lookupDstOffset;
extern long   gmtOffset_sec;
extern int    daylightOffset_sec;
extern bool   regionAutoMode;
extern bool   manualDstActive;

// Clock / weather timing
extern int          lastDay;
extern unsigned long lastWeatherUpdate;

// ---------------------------------------------------------------------------
String syncRegion() {
    if ( WiFi.status() != WL_CONNECTED ) {
        return "No WiFi connection";
    }

    log_d( "[AUTO] Syncing region..." );

    HTTPClient http;
    http.setTimeout( HTTP_TIMEOUT_STANDARD );
    http.begin( "http://ip-api.com/json?fields=status,city,timezone,lat,lon" );

    int httpCode = http.GET();
    if ( httpCode == 200 ) {
        JsonDocument doc;
        DeserializationError error = deserializeJson( doc, http.getString() );

        if ( !error && doc[ "status" ] == "success" ) {
            // 1. Get data from API into local variables
            String detectedCity = doc[ "city" ].as<String>();
            String detectedTimezone = doc[ "timezone" ].as<String>();
            float detectedLat = doc[ "lat" ].as<float>();
            float detectedLon = doc[ "lon" ].as<float>();

            log_i( "[AUTO] Detected: %s, TZ: %s", detectedCity.c_str(), detectedTimezone.c_str() );
            log_d( "[AUTO] Coordinates from IP: %.4f, %.4f", detectedLat, detectedLon );

            // 2. Set global 'selected' variables for applyLocation
            selectedCity = detectedCity;
            selectedTimezone = detectedTimezone;

            // Detect country from timezone
            if ( detectedTimezone.indexOf( "Prague" ) >= 0 ) {
                selectedCountry = "Czech Republic";
            }
            else if ( detectedTimezone.indexOf( "Berlin" ) >= 0 ) {
                selectedCountry = "Germany";
            }
            else if ( detectedTimezone.indexOf( "Warsaw" ) >= 0 ) {
                selectedCountry = "Poland";
            }
            else if ( detectedTimezone.indexOf( "Bratislava" ) >= 0 ) {
                selectedCountry = "Slovakia";
            }
            else if ( detectedTimezone.indexOf( "Paris" ) >= 0 ) {
                selectedCountry = "France";
            }
            else if ( detectedTimezone.indexOf( "London" ) >= 0 ) {
                selectedCountry = "United Kingdom";
            }
            else if ( detectedTimezone.indexOf( "New_York" ) >= 0 || detectedTimezone.indexOf( "Chicago" ) >= 0 ||
                      detectedTimezone.indexOf( "Denver" ) >= 0 || detectedTimezone.indexOf( "Los_Angeles" ) >= 0 ) {
                selectedCountry = "United States";
            }
            else if ( detectedTimezone.indexOf( "Tokyo" ) >= 0 ) {
                selectedCountry = "Japan";
            }
            else if ( detectedTimezone.indexOf( "Shanghai" ) >= 0 || detectedTimezone.indexOf( "Hong_Kong" ) >= 0 ) {
                selectedCountry = "China";
            }
            else if ( detectedTimezone.indexOf( "Sydney" ) >= 0 || detectedTimezone.indexOf( "Melbourne" ) >= 0 ||
                      detectedTimezone.indexOf( "Brisbane" ) >= 0 || detectedTimezone.indexOf( "Adelaide" ) >= 0 ||
                      detectedTimezone.indexOf( "Perth" ) >= 0 ) {
                selectedCountry = "Australia";
            }
            else {
                // Fallback - leave selectedCountry as-is if auto-detection can't determine it
            }
            // Detect POSIX TZ via timeapi.io from coordinates (works anywhere in the world)
            if ( detectedLat != 0.0 || detectedLon != 0.0 ) {
                detectTimezoneFromCoords( detectedLat, detectedLon, selectedCountry );
                log_i( "[AUTO] POSIX TZ from timeapi.io: %s", posixTZ.c_str() );
            }
            else {
                posixTZ = ianaToPostfixTZ( detectedTimezone );
                log_d( "[AUTO] POSIX TZ from lookup table (no coords): %s", posixTZ.c_str() );
            }

            log_d( "[AUTO] SelectedCountry set to: %s", selectedCountry.c_str() );

            // 3. APPLY CHANGES (saves, sets time - internally resets lat/lon to 0.0)
            applyLocation();

            // 4. OVERWRITE ZEROS with real coordinates from IP geolocation
            //    applyLocation() intentionally resets lat/lon, so we must restore them after
            if ( detectedLat != 0.0 || detectedLon != 0.0 ) {
                lat = detectedLat;
                lon = detectedLon;
                prefs.begin( "sys", false );
                prefs.putFloat( "lat", lat );
                prefs.putFloat( "lon", lon );
                prefs.end();
                log_d( "[AUTO] Coordinates saved: %.4f, %.4f", lat, lon );
            }

        }
        else {
            log_e( "[AUTO] JSON Parsing error or status not success" );
            http.end();
            return "Location API failed";
        }
    }
    else {
        log_w( "[AUTO] HTTP Error: %d", httpCode );
        http.end();
        return "Server error (HTTP " + String( httpCode ) + ")";
    }
    http.end();
    return "";
}

// drawArrowDown -> src/ui/icons.cpp

// drawArrowUp -> src/ui/icons.cpp

// showWifiConnectingScreen -> src/ui/screens.cpp

// showWifiResultScreen -> src/ui/screens.cpp

// scanWifiNetworks -> src/ui/screens.cpp

// drawSettingsScreen -> src/ui/screens.cpp

// drawWeatherScreen -> src/ui/screens.cpp

// drawCoordInputScreen -> src/ui/screens.cpp

// drawRegionalScreen -> src/ui/screens.cpp

// drawCountrySelection -> src/ui/screens.cpp

// drawCitySelection -> src/ui/screens.cpp

// drawLocationConfirm -> src/ui/screens.cpp

// drawCountryLookupConfirm -> src/ui/screens.cpp

// drawCityLookupConfirm -> src/ui/screens.cpp

// drawCustomCityInput -> src/ui/screens.cpp

// drawCustomCountryInput -> src/ui/screens.cpp

// ================= FIRMWARE SETTINGS SCREEN =================

// drawFirmwareScreen -> src/ui/screens.cpp

// drawGraphicsScreen -> src/ui/screens.cpp

// drawInitialSetup -> src/ui/screens.cpp

// drawKeyboardScreen -> src/ui/screens.cpp

// updateKeyboardText -> src/ui/screens.cpp

// drawClockStatic -> src/ui/clock_face.cpp

// drawClockFace -> src/ui/clock_face.cpp

// drawDateAndWeek -> src/ui/clock_face.cpp

// drawDigitalClock -> src/ui/clock_face.cpp

// updateHands -> src/ui/clock_face.cpp

// ianaToPostfixTZ -> src/net/timezone.cpp

// ============================================
// FIX 3: Save and load coordinates
// ============================================
void applyLocation() {
    // Try ianaToPostfixTZ (knows Europe, major US cities, etc.)
    // If it returns "UTC0" for a non-UTC zone, the zone is unknown.
    // In that case keep the posixTZ set by detectTimezoneFromCoords() from timeapi.io.
    String candidate = ianaToPostfixTZ( selectedTimezone );
    bool isUnknownZone = ( candidate == "UTC0" &&
                           selectedTimezone != "" &&
                           selectedTimezone.indexOf( "UTC" ) < 0 &&
                           selectedTimezone.indexOf( "GMT" ) < 0 );
    if ( !isUnknownZone ) {
        posixTZ = candidate;
    }
    log_d( "[TZ] Applying POSIX TZ: %s for IANA: %s", posixTZ.c_str(), selectedTimezone.c_str() );
    configTime( 0, 0, ntpServer );
    setenv( "TZ", posixTZ.c_str(), 1 );
    tzset();

    // RESET COORDINATES - when selecting from list we must let fetchWeatherData() find the new coordinates
    lat = 0.0;
    lon = 0.0;

    // Save to preferences
    prefs.begin( "sys", false );
    prefs.putString( "city", selectedCity );
    prefs.putString( "country", selectedCountry );
    prefs.putString( "timezone", selectedTimezone );
    prefs.putString( "posixTZ", posixTZ );
    prefs.putInt( "gmt", gmtOffset_sec );
    prefs.putInt( "dst", daylightOffset_sec );
    manualDstActive = false;
    prefs.putBool( "manualDst", false );
    prefs.putString( "isoCode", lookupISOCode );

    // ALSO SAVE COORDINATES (now 0.0 so weather update will fetch correct ones next time)
    prefs.putFloat( "lat", lat );
    prefs.putFloat( "lon", lon );

    prefs.end();
    cityName = selectedCity;

    lastDay = -1; // Force date redraw
    lastWeatherUpdate = 0; // Force weather update
    lastNamedayDay = -1; // Force nameday update on location change
    handleNamedayUpdate(); // Update nameday immediately after location change
    lastHolidayDay = -1; // Force holiday update on location change
    handleHolidayUpdate(); // Update holiday immediately after location change
}

void loadSavedLocation() {
    prefs.begin( "sys", false );
    regionAutoMode = prefs.getBool( "regionAuto", true );
    manualDstActive = prefs.getBool( "manualDst", false );
    String savedCountry = prefs.getString( "country", "" );
    String savedCity = prefs.getString( "city", "" );
    selectedTimezone = prefs.getString( "timezone", "" );
    posixTZ = prefs.getString( "posixTZ", "CET-1CEST,M3.5.0,M10.5.0/3" );
    lookupISOCode = prefs.getString( "isoCode", "" );  // Persisted ISO code — avoids REST lookup on boot

    // FIX: Unified key names with applyLocation ("gmt" instead of "gmtOffset")
    gmtOffset_sec = prefs.getInt( "gmt", 3600 );
    daylightOffset_sec = prefs.getInt( "dst", 3600 );

    // LOAD SAVED COORDINATES
    lat = prefs.getFloat( "lat", 0.0 );
    lon = prefs.getFloat( "lon", 0.0 );

    prefs.end();

    if ( savedCity != "" ) {
        cityName = savedCity;
        selectedCity = savedCity;
        selectedCountry = savedCountry;
        // Populate lookupISOCode from the embedded table so holiday lookup
        // works on the first run without waiting for a new country selection.
        lookupCountryEmbedded( savedCountry );
        // Sanity check: if country is Czech Republic but timezone is non-European,
        // the value was the old hardcoded default and should not be used.
        if ( selectedCountry == "Czech Republic" &&
                selectedTimezone != "" &&
                !selectedTimezone.startsWith( "Europe/" ) ) {
            selectedCountry = "";
            log_d( "[LOAD] Cleared stale Czech Republic country default (timezone: %s)", selectedTimezone.c_str() );
        }
        // If posixTZ was not saved (old firmware version), derive it from IANA timezone
        if ( posixTZ == "" || posixTZ == "CET-1CEST,M3.5.0,M10.5.0/3" ) {
            if ( selectedTimezone != "" ) {
                posixTZ = ianaToPostfixTZ( selectedTimezone );
            }
        }
        log_d( "[LOAD] Applying POSIX TZ: %s", posixTZ.c_str() );
        configTime( 0, 0, ntpServer );
        setenv( "TZ", posixTZ.c_str(), 1 );
        tzset();
        log_i( "[LOAD] Location loaded: %s (%.4f,%.4f)", cityName.c_str(), lat, lon );
    }
}

