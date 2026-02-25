#include "weather_api.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "../util/constants.h"
#include "../data/app_state.h"
#include "timezone.h"

// ---------------------------------------------------------------------------
// Externs – all defined in main.cpp
// ---------------------------------------------------------------------------
extern float       lat;
extern float       lon;
extern String      weatherCity;
extern String      selectedCountry;
extern Preferences prefs;

extern String      posixTZ;
extern int         lookupGmtOffset;
extern int         lookupDstOffset;

extern const char *ntpServer;
extern int         lastDay;

extern float       currentTemp;
extern int         currentHumidity;
extern int         weatherCode;
extern float       currentWindSpeed;
extern int         currentWindDirection;
extern int         currentPressure;

extern ForecastData forecast[ 2 ];
extern String       forecastDay1Name;
extern String       forecastDay2Name;

extern String       sunriseTime;
extern String       sunsetTime;

extern bool         initialWeatherFetched;

// ---------------------------------------------------------------------------

String getWeatherDesc( int code ) {
    if ( code == 0 ) {
        return "Clear";
    }
    if ( code <= 3 ) {
        return "Cloudy";
    }
    if ( code <= 48 ) {
        return "Fog";
    }
    if ( code <= 67 ) {
        return "Rain";
    }
    if ( code <= 77 ) {
        return "Snow";
    }
    if ( code <= 82 ) {
        return "Showers";
    }
    if ( code <= 99 ) {
        return "Storm";
    }
    return "Unknown";
}

String getWindDir( int deg ) {
    if ( deg >= 337 || deg < 22 ) {
        return "N";
    }
    if ( deg < 67 ) {
        return "NE";
    }
    if ( deg < 112 ) {
        return "E";
    }
    if ( deg < 157 ) {
        return "SE";
    }
    if ( deg < 202 ) {
        return "S";
    }
    if ( deg < 247 ) {
        return "SW";
    }
    if ( deg < 292 ) {
        return "W";
    }
    return "NW";
}

// ============================================
// FIX 4: Use accurate coordinates for weather
// ============================================
void fetchWeatherData() {
    if ( WiFi.status() != WL_CONNECTED ) {
        return;
    }

    HTTPClient http;

    // STEP 1: Get coordinates
    // If we already have coordinates from Custom Lookup (not 0.0), USE THEM and don't search again
    if ( lat != 0.0 && lon != 0.0 ) {
        log_d( "[WEATHER] Using saved coordinates: %.4f, %.4f", lat, lon );
    }
    else {
        // No coordinates yet (e.g. selected from embedded city list), must look them up
        // Smarter search: fetch multiple results and filter by country
        log_d( "[WEATHER] Searching coordinates for: %s, Country: %s", weatherCity.c_str(), selectedCountry.c_str() );

        String searchName = weatherCity;
        searchName.replace( " ", "+" );
        String geoUrl = "https://geocoding-api.open-meteo.com/v1/search?name=" + searchName + "&count=5&language=en&format=json";

        http.setTimeout( HTTP_TIMEOUT_SHORT );
        http.begin( geoUrl );
        int httpCode = http.GET();

        if ( httpCode == 200 ) {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson( doc, payload );

            bool found = false;
            if ( doc[ "results" ].size() > 0 ) {
                // Scan results and try to find a country match
                for ( JsonVariant result : doc[ "results" ].as<JsonArray>() ) {
                    String resCountry = result[ "country" ].as<String>();
                    String resCode = result[ "country_code" ].as<String>();

                    // Compare country (fuzzy match)
                    if ( resCountry.indexOf( selectedCountry ) >= 0 || selectedCountry.indexOf( resCountry ) >= 0 ||
                            resCode.equalsIgnoreCase( selectedCountry ) ) {

                        lat = result[ "latitude" ];
                        lon = result[ "longitude" ];
                        log_d( "[WEATHER] Match found: %s, %s", result[ "name" ].as<String>().c_str(), resCountry.c_str() );
                        found = true;
                        break;
                    }
                }

                // No country match found, take first result (fallback)
                if ( !found ) {
                    lat = doc[ "results" ][ 0 ][ "latitude" ];
                    lon = doc[ "results" ][ 0 ][ "longitude" ];
                    log_w( "[WEATHER] Country match failed, taking first result: %s", doc[ "results" ][ 0 ][ "country" ].as<String>().c_str() );
                }

                // Save new coordinates so we don't need to search again next time
                prefs.begin( "sys", false );
                prefs.putFloat( "lat", lat );
                prefs.putFloat( "lon", lon );
                prefs.end();
            }
        }
        http.end();
    }

    // STEP 1b: Refresh timezone from timeapi.io (every 30 min = every weather update)
    // This automatically corrects DST transitions anywhere in the world
    if ( lat != 0.0 || lon != 0.0 ) {
        String oldPosix = posixTZ;
        detectTimezoneFromCoords( lat, lon, selectedCountry );
        // detectTimezoneFromCoords set the global posixTZ
        // Apply it to the ESP32 clock
        configTime( 0, 0, ntpServer );
        setenv( "TZ", posixTZ.c_str(), 1 );
        tzset();
        // Save to flash only if changed (DST transition)
        if ( posixTZ != oldPosix ) {
            log_i( "[WEATHER] Timezone changed: %s -> %s", oldPosix.c_str(), posixTZ.c_str() );
            prefs.begin( "sys", false );
            prefs.putString( "posixTZ", posixTZ );
            prefs.putInt( "gmt", lookupGmtOffset );
            prefs.putInt( "dst", lookupDstOffset );
            prefs.end();
            lastDay = -1; // Force date/day redraw
        }
        else {
            log_d( "[WEATHER] Timezone unchanged: %s", posixTZ.c_str() );
        }
    }

    // STEP 2: Fetch weather for these coordinates
    time_t now = time( nullptr );
    struct tm *timeinfo = localtime( &now );

    if ( timeinfo ) {
        const char *dayAbbr[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        int tomorrowWday = ( timeinfo->tm_wday + 1 ) % 7;
        forecastDay1Name = dayAbbr[ tomorrowWday ];
        int afterTomorrowWday = ( timeinfo->tm_wday + 2 ) % 7;
        forecastDay2Name = dayAbbr[ afterTomorrowWday ];
    }

    String weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude=" + String( lat, 4 ) + "&longitude=" + String( lon, 4 ) +
                        "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,pressure_msl&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset&timezone=auto";

    http.setTimeout( HTTP_TIMEOUT_STANDARD );
    http.begin( weatherUrl );
    int httpCode = http.GET();

    if ( httpCode == 200 ) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson( doc, payload );

        if ( !error ) {
            currentTemp = doc[ "current" ][ "temperature_2m" ];
            currentHumidity = doc[ "current" ][ "relative_humidity_2m" ];
            weatherCode = doc[ "current" ][ "weather_code" ];
            currentWindSpeed = doc[ "current" ][ "wind_speed_10m" ];
            currentWindDirection = doc[ "current" ][ "wind_direction_10m" ];

            forecast[ 0 ].code = doc[ "daily" ][ "weather_code" ][ 1 ];
            forecast[ 0 ].tempMax = doc[ "daily" ][ "temperature_2m_max" ][ 1 ];
            forecast[ 0 ].tempMin = doc[ "daily" ][ "temperature_2m_min" ][ 1 ];
            forecast[ 1 ].code = doc[ "daily" ][ "weather_code" ][ 2 ];
            forecast[ 1 ].tempMax = doc[ "daily" ][ "temperature_2m_max" ][ 2 ];
            forecast[ 1 ].tempMin = doc[ "daily" ][ "temperature_2m_min" ][ 2 ];

            // Sunrise/Sunset processing
            if ( doc[ "daily" ][ "sunrise" ].size() > 0 ) {
                String sunriseRaw = doc[ "daily" ][ "sunrise" ][ 0 ].as<String>();
                int tPos = sunriseRaw.indexOf( 'T' );
                if ( tPos > 0 ) {
                    sunriseTime = sunriseRaw.substring( tPos + 1, tPos + 6 );
                }
            }
            if ( doc[ "daily" ][ "sunset" ].size() > 0 ) {
                String sunsetRaw = doc[ "daily" ][ "sunset" ][ 0 ].as<String>();
                int tPos = sunsetRaw.indexOf( 'T' );
                if ( tPos > 0 ) {
                    sunsetTime = sunsetRaw.substring( tPos + 1, tPos + 6 );
                }
            }

            // Pressure processing
            if ( doc[ "current" ][ "pressure_msl" ] ) {
                currentPressure = doc[ "current" ][ "pressure_msl" ].as<int>();
            }
            else {
                currentPressure = 1013;
            }

            initialWeatherFetched = true;
            log_i( "[WEATHER] Data fetched successfully" );
        }
        else {
            log_e( "[WEATHER] JSON error: %s", error.c_str() );
        }
    }
    else {
        log_w( "[WEATHER] HTTP Error: %d", httpCode );
    }
    http.end();
}
