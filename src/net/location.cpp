#include "location.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "../util/constants.h"
#include "../util/string_utils.h"
#include "../data/city_data.h"
#include "timezone.h"

// Externs defined in main.cpp
extern String lookupCountry;
extern String lookupISOCode;
extern String lookupCity;
extern String lookupTimezone;
extern int    lookupGmtOffset;
extern int    lookupDstOffset;
extern float  lat;
extern float  lon;
extern float  lookupLat;
extern float  lookupLon;

bool lookupCountryRESTAPI( String countryName ) {
    if ( WiFi.status() != WL_CONNECTED ) {
        log_w( "[LOOKUP-REST] WiFi not connected" );
        return false;
    }
    countryName = toTitleCase( countryName );
    log_d( "[LOOKUP-REST] Searching REST API %s", countryName.c_str() );

    HTTPClient http;
    http.setTimeout( HTTP_TIMEOUT_GEO );

    String searchName = countryName;
    searchName.replace( " ", "%20" );
    String url = "https://restcountries.com/v3.1/name/" + searchName + "?fullText=false&fields=name,cca2";
    log_d( "[LOOKUP-REST] URL %s", url.c_str() );

    http.begin( url );
    http.setUserAgent( "ESP32" );

    int httpCode = http.GET();
    log_d( "[LOOKUP-REST] HTTP Code %d", httpCode );

    if ( httpCode != 200 ) {
        log_w( "[LOOKUP-REST] HTTP Error %d", httpCode );
        http.end();
        return false;
    }

    String response = http.getString();

    JsonDocument doc;
    DeserializationError error = deserializeJson( doc, response );
    if ( error ) {
        log_e( "[LOOKUP-REST] JSON error %s", error.c_str() );
        http.end();
        return false;
    }

    if ( doc.is<JsonArray>() ) {
        JsonArray arr = doc.as<JsonArray>();
        if ( arr.size() > 0 ) {
            JsonObject first = arr[ 0 ];
            if ( first[ "name" ].is<JsonObject>() ) {
                JsonObject nameObj = first[ "name" ];
                if ( nameObj[ "common" ].is<const char * >() ) {
                    lookupCountry = nameObj[ "common" ].as<String>();
                    lookupISOCode = first[ "cca2" ].is<const char *>()
                                    ? first[ "cca2" ].as<String>()
                                    : String( "" );
                    log_d( "[LOOKUP-REST] FOUND %s (%s)", lookupCountry.c_str(), lookupISOCode.c_str() );
                    http.end();
                    return true;
                }
            }
        }
    }

    log_w( "[LOOKUP-REST] HTTP Error %d", httpCode );
    http.end();
    return false;
}

bool lookupCountryEmbedded( String countryName ) {
    countryName = toTitleCase( countryName );
    log_d( "[LOOKUP-EMB] Searching embedded %s", countryName.c_str() );
    for ( int i = 0; i < COUNTRIES_COUNT; i++ ) {
        if ( fuzzyMatch( countryName, String( countries[ i ].name ) ) ) {
            lookupCountry = String( countries[ i ].name );
            lookupISOCode = String( countries[ i ].code );
            log_d( "[LOOKUP-EMB] FOUND %s (%s)", lookupCountry.c_str(), lookupISOCode.c_str() );
            return true;
        }
    }
    return false;
}

bool lookupCountryGeonames( String countryName ) {
    if ( lookupCountryEmbedded( countryName ) ) {
        return true;
    }
    if ( WiFi.status() == WL_CONNECTED ) {
        if ( lookupCountryRESTAPI( countryName ) ) {
            return true;
        }
    }
    return false;
}

// ============================================
// FIX 1: Get timezone from API (worldwide)
// ============================================
// detectTimezoneFromCoords -> src/net/timezone.cpp

// ============================================
// FIX 2: Save to GLOBAL coordinates
// ============================================
bool lookupCityNominatim( String cityName, String countryHint ) {
    if ( WiFi.status() != WL_CONNECTED ) {
        log_w( "[LOOKUP-CITY-NOM] WiFi not connected" );
        return false;
    }
    cityName = toTitleCase( cityName );
    log_d( "[LOOKUP-CITY-NOM] Searching %s in %s", cityName.c_str(), countryHint.c_str() );

    HTTPClient http;
    http.setTimeout( HTTP_TIMEOUT_NOMINATIM );

    String searchCity = cityName;
    searchCity.replace( " ", "%20" );
    String searchCountry = countryHint;
    searchCountry.replace( " ", "%20" );
    String url = "https://nominatim.openstreetmap.org/search?format=json&addressdetails=1&limit=1&q=" + searchCity + "%2C" + searchCountry;
    log_d( "[LOOKUP-CITY-NOM] URL %s", url.c_str() );

    http.begin( url );
    http.addHeader( "User-Agent", "ESP32-DataDisplay/1.0" ); // Nominatim requires a User-Agent header
    int httpCode = http.GET();

    if ( httpCode == 200 ) {
        String response = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson( doc, response );
        if ( error ) {
            log_e( "[LOOKUP-CITY-NOM] JSON error" );
            http.end();
            return false;
        }

        if ( doc.is<JsonArray>() ) {
            JsonArray arr = doc.as<JsonArray>();
            if ( arr.size() > 0 ) {
                JsonObject first = arr[ 0 ];
                if ( first[ "name" ].is<const char * >() && first[ "lat" ].is<const char * >() && first[ "lon" ].is<const char * >() ) {
                    String apiName = first[ "name" ].as<String>();
                    // Nominatim may return the name in the local script (Arabic, Chinese...)
                    // If it contains non-ASCII characters, use the name entered by the user
                    bool isAscii = true;
                    for ( int ci = 0; ci < ( int )apiName.length(); ci++ ) {
                        if ( ( unsigned char )apiName[ ci ] > 127 ) {
                            isAscii = false;
                            break;
                        }
                    }
                    lookupCity = isAscii ? apiName : cityName;

                    // Store to global variables and lookup backup
                    const char *latStr = first[ "lat" ].as<const char *>();
                    const char *lonStr = first[ "lon" ].as<const char *>();
                    if ( latStr && lonStr ) {
                        lat = atof( latStr );
                        lon = atof( lonStr );
                        lookupLat = lat;
                        lookupLon = lon;
                    }

                    log_i( "[LOOKUP-CITY-NOM] FOUND %s Lat %.4f, Lon %.4f", lookupCity.c_str(), lat, lon );

                    // Call timezone detection with found coordinates
                    detectTimezoneFromCoords( lat, lon, countryHint );

                    log_d( "[LOOKUP-CITY-NOM] Timezone set %s", lookupTimezone.c_str() );
                    http.end();
                    return true;
                }
            }
        }
    }
    http.end();
    return false;
}

bool lookupCityGeonames( String cityName, String countryHint ) {
    cityName = toTitleCase( cityName );
    log_d( "[LOOKUP-CITY] Searching %s in %s", cityName.c_str(), countryHint.c_str() );

    for ( int i = 0; i < COUNTRIES_COUNT; i++ ) {
        if ( String( countries[ i ].name ) == countryHint || String( countries[ i ].code ) == countryHint ) {
            for ( int j = 0; j < countries[ i ].cityCount; j++ ) {
                if ( fuzzyMatch( cityName, countries[ i ].cities[ j ].name ) ) {
                    lookupCity = countries[ i ].cities[ j ].name;
                    lookupTimezone = countries[ i ].cities[ j ].timezone;
                    lookupGmtOffset = countries[ i ].cities[ j ].gmtOffset;
                    lookupDstOffset = countries[ i ].cities[ j ].dstOffset;
                    log_i( "[LOOKUP-CITY] FOUND in embedded %s", lookupCity.c_str() );
                    return true;
                }
            }
        }
    }

    log_d( "[LOOKUP-CITY] NOT in embedded DB, trying Nominatim API..." );
    if ( WiFi.status() == WL_CONNECTED ) {
        if ( lookupCityNominatim( cityName, countryHint ) ) {
            log_i( "[LOOKUP-CITY] FOUND via Nominatim" );
            return true;
        }
        else {
            log_w( "[LOOKUP-CITY] WiFi not connected, cannot use Nominatim" );
        }
    }
    log_w( "[LOOKUP-CITY] NOT FOUND anywhere" );
    return false;
}

