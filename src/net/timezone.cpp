#include "timezone.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "../util/constants.h"

// Globals owned by main.cpp
extern String lookupTimezone;
extern int    lookupGmtOffset;
extern int    lookupDstOffset;
extern String posixTZ;

// ============================================================
// ianaToPostfixTZ
// ============================================================

String ianaToPostfixTZ( String iana ) {
    if ( iana.indexOf( "Prague" ) >= 0 || iana.indexOf( "Berlin" ) >= 0 ||
            iana.indexOf( "Warsaw" ) >= 0 || iana.indexOf( "Vienna" ) >= 0 ||
            iana.indexOf( "Bratislava" ) >= 0 || iana.indexOf( "Paris" ) >= 0 ||
            iana.indexOf( "Rome" ) >= 0 || iana.indexOf( "Madrid" ) >= 0 ||
            iana.indexOf( "Amsterdam" ) >= 0 || iana.indexOf( "Brussels" ) >= 0 ||
            iana.indexOf( "Budapest" ) >= 0 || iana.indexOf( "Copenhagen" ) >= 0 ||
            iana.indexOf( "Oslo" ) >= 0 || iana.indexOf( "Stockholm" ) >= 0 ||
            iana.indexOf( "Zurich" ) >= 0 || iana.indexOf( "Belgrade" ) >= 0 ||
            iana.indexOf( "Ljubljana" ) >= 0 || iana.indexOf( "Zagreb" ) >= 0 ) {
        return "CET-1CEST,M3.5.0,M10.5.0/3";
    }
    if ( iana.indexOf( "London" ) >= 0 || iana.indexOf( "Dublin" ) >= 0 ||
            iana.indexOf( "Lisbon" ) >= 0 ) {
        return "GMT0BST,M3.5.0/1,M10.5.0";
    }
    if ( iana.indexOf( "Helsinki" ) >= 0 || iana.indexOf( "Kyiv" ) >= 0 ||
            iana.indexOf( "Kiev" ) >= 0 || iana.indexOf( "Riga" ) >= 0 ||
            iana.indexOf( "Tallinn" ) >= 0 || iana.indexOf( "Vilnius" ) >= 0 ||
            iana.indexOf( "Sofia" ) >= 0 || iana.indexOf( "Bucharest" ) >= 0 ||
            iana.indexOf( "Athens" ) >= 0 ) {
        return "EET-2EEST,M3.5.0/3,M10.5.0/4";
    }
    if ( iana.indexOf( "Moscow" ) >= 0 ) {
        return "MSK-3";
    }
    if ( iana.indexOf( "Istanbul" ) >= 0 ) {
        return "TRT-3";
    }
    if ( iana.indexOf( "New_York" ) >= 0 || iana.indexOf( "Toronto" ) >= 0 ||
            iana.indexOf( "Montreal" ) >= 0 ) {
        return "EST5EDT,M3.2.0,M11.1.0";
    }
    if ( iana.indexOf( "Chicago" ) >= 0 || iana.indexOf( "Winnipeg" ) >= 0 ) {
        return "CST6CDT,M3.2.0,M11.1.0";
    }
    if ( iana.indexOf( "Denver" ) >= 0 || iana.indexOf( "Edmonton" ) >= 0 || iana.indexOf( "Boise" ) >= 0 ) {
        return "MST7MDT,M3.2.0,M11.1.0";
    }
    if ( iana.indexOf( "Phoenix" ) >= 0 ) {
        return "MST7";
    }
    if ( iana.indexOf( "Los_Angeles" ) >= 0 || iana.indexOf( "Vancouver" ) >= 0 ) {
        return "PST8PDT,M3.2.0,M11.1.0";
    }
    if ( iana.indexOf( "Anchorage" ) >= 0 ) {
        return "AKST9AKDT,M3.2.0,M11.1.0";
    }
    if ( iana.indexOf( "Honolulu" ) >= 0 ) {
        return "HST10";
    }
    if ( iana.indexOf( "Tokyo" ) >= 0 ) {
        return "JST-9";
    }
    if ( iana.indexOf( "Seoul" ) >= 0 ) {
        return "KST-9";
    }
    if ( iana.indexOf( "Shanghai" ) >= 0 || iana.indexOf( "Hong_Kong" ) >= 0 ||
            iana.indexOf( "Taipei" ) >= 0 || iana.indexOf( "Singapore" ) >= 0 ||
            iana.indexOf( "Kuala_Lumpur" ) >= 0 || iana.indexOf( "Perth" ) >= 0 ) {
        return "CST-8";
    }
    if ( iana.indexOf( "Kolkata" ) >= 0 || iana.indexOf( "Calcutta" ) >= 0 ) {
        return "IST-5:30";
    }
    if ( iana.indexOf( "Dubai" ) >= 0 || iana.indexOf( "Muscat" ) >= 0 ) {
        return "GST-4";
    }
    if ( iana.indexOf( "Riyadh" ) >= 0 || iana.indexOf( "Baghdad" ) >= 0 ||
            iana.indexOf( "Kuwait" ) >= 0 ) {
        return "AST-3";
    }
    if ( iana.indexOf( "Tehran" ) >= 0 ) {
        return "IRST-3:30IRDT,80/0,264/0";
    }
    if ( iana.indexOf( "Sydney" ) >= 0 || iana.indexOf( "Melbourne" ) >= 0 ||
            iana.indexOf( "Hobart" ) >= 0 || iana.indexOf( "Canberra" ) >= 0 ||
            iana.indexOf( "Wollongong" ) >= 0 ) {
        return "AEST-10AEDT,M10.1.0,M4.1.0/3";
    }
    if ( iana.indexOf( "Brisbane" ) >= 0 || iana.indexOf( "Townsville" ) >= 0 ) {
        return "AEST-10";
    }
    if ( iana.indexOf( "Adelaide" ) >= 0 ) {
        return "ACST-9:30ACDT,M10.1.0,M4.1.0/3";
    }
    if ( iana.indexOf( "Darwin" ) >= 0 ) {
        return "ACST-9:30";
    }
    if ( iana.indexOf( "Auckland" ) >= 0 ) {
        return "NZST-12NZDT,M9.5.0,M4.1.0/3";
    }
    if ( iana.indexOf( "Sao_Paulo" ) >= 0 || iana.indexOf( "Buenos_Aires" ) >= 0 ) {
        return "BRT3";
    }
    // Africa
    if ( iana.indexOf( "Cairo" ) >= 0 ) {
        return "EET-2";  // Egypt UTC+2, no DST
    }
    if ( iana.indexOf( "Johannesburg" ) >= 0 || iana.indexOf( "Harare" ) >= 0 ||
            iana.indexOf( "Lusaka" ) >= 0 || iana.indexOf( "Maputo" ) >= 0 ||
            iana.indexOf( "Gaborone" ) >= 0 || iana.indexOf( "Maseru" ) >= 0 ||
            iana.indexOf( "Mbabane" ) >= 0 || iana.indexOf( "Bulawayo" ) >= 0 ) {
        return "CAT-2";  // South/Central Africa UTC+2, no DST
    }
    if ( iana.indexOf( "Nairobi" ) >= 0 || iana.indexOf( "Addis_Ababa" ) >= 0 ||
            iana.indexOf( "Dar_es_Salaam" ) >= 0 || iana.indexOf( "Kampala" ) >= 0 ||
            iana.indexOf( "Mogadishu" ) >= 0 || iana.indexOf( "Antananarivo" ) >= 0 ) {
        return "EAT-3";  // East Africa UTC+3, no DST
    }
    if ( iana.indexOf( "Lagos" ) >= 0 || iana.indexOf( "Kinshasa" ) >= 0 ||
            iana.indexOf( "Douala" ) >= 0 || iana.indexOf( "Libreville" ) >= 0 ||
            iana.indexOf( "Luanda" ) >= 0 || iana.indexOf( "Bangui" ) >= 0 ||
            iana.indexOf( "Brazzaville" ) >= 0 || iana.indexOf( "Malabo" ) >= 0 ) {
        return "WAT-1";  // West/Central Africa UTC+1, no DST
    }
    if ( iana.indexOf( "Abidjan" ) >= 0 || iana.indexOf( "Accra" ) >= 0 ||
            iana.indexOf( "Dakar" ) >= 0 || iana.indexOf( "Bamako" ) >= 0 ||
            iana.indexOf( "Conakry" ) >= 0 || iana.indexOf( "Freetown" ) >= 0 ||
            iana.indexOf( "Monrovia" ) >= 0 || iana.indexOf( "Ouagadougou" ) >= 0 ) {
        return "GMT0";  // West Africa UTC+0, no DST
    }
    if ( iana.indexOf( "Casablanca" ) >= 0 || iana.indexOf( "El_Aaiun" ) >= 0 ) {
        return "WET0WEST,M3.5.0,M10.5.0/3";  // Morocco has DST
    }
    if ( iana.indexOf( "Tunis" ) >= 0 ) {
        return "CET-1";  // Tunisia UTC+1, no DST
    }
    if ( iana.indexOf( "Tripoli" ) >= 0 ) {
        return "EET-2";  // Libya UTC+2, no DST
    }
    if ( iana.indexOf( "Khartoum" ) >= 0 ) {
        return "CAT-3";  // Sudan UTC+3, no DST
    }
    log_w( "[TZ] Unknown IANA zone: %s, fallback UTC", iana.c_str() );
    return "UTC0";
}

// ============================================================
// detectTimezoneFromCoords
// ============================================================

void detectTimezoneFromCoords( float lat, float lon, String countryHint ) {
    if ( WiFi.status() != WL_CONNECTED ) {
        log_w( "[TZ-AUTO] WiFi not connected, using fallback" );
        lookupTimezone = "Europe/Prague";
        lookupGmtOffset = 3600;
        lookupDstOffset = 3600;
        posixTZ = "CET-1CEST,M3.5.0,M10.5.0/3";
        return;
    }

    log_d( "[TZ-AUTO] Detecting timezone via timeapi.io for: %.4f, %.4f", lat, lon );

    HTTPClient http;
    String url = "https://timeapi.io/api/timezone/coordinate?latitude=" + String( lat, 4 ) + "&longitude=" + String( lon, 4 );

    http.setTimeout( HTTP_TIMEOUT_GEO );
    http.begin( url );
    http.addHeader( "Accept", "application/json" );
    int httpCode = http.GET();

    if ( httpCode == 200 ) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson( doc, payload );

        if ( !error ) {
            // 1. Get IANA timezone name
            String ianaName = "";
            if ( doc[ "timeZone" ] ) {
                ianaName = doc[ "timeZone" ].as<String>();
            }

            // 2. Get current UTC offset in seconds
            int currentOffset = 0;
            if ( doc[ "currentUtcOffset" ][ "seconds" ] ) {
                currentOffset = doc[ "currentUtcOffset" ][ "seconds" ].as<int>();
            }

            // 3. Get standard offset (without DST)
            int standardOffset = currentOffset;
            if ( doc[ "standardUtcOffset" ][ "seconds" ] ) {
                standardOffset = doc[ "standardUtcOffset" ][ "seconds" ].as<int>();
            }

            // 4. Has DST?
            bool hasDst = doc[ "hasDayLightSaving" ].as<bool>();

            log_d( "[TZ-AUTO] Found: %s currentOffset: %ds, hasDST: %d", ianaName.c_str(), currentOffset, hasDst );

            // Store data
            lookupTimezone = ( ianaName != "" ) ? ianaName : "UTC";
            lookupGmtOffset = currentOffset;
            lookupDstOffset = 0;

            // 5. Try ianaToPostfixTZ for accurate DST rules (works for Europe, major US cities, etc.)
            String newPosix = "";
            if ( ianaName != "" ) {
                newPosix = ianaToPostfixTZ( ianaName );
            }

            // 6. If ianaToPostfixTZ returns unknown zone ("UTC0" for a non-UTC zone),
            //    build POSIX string directly from the offset
            bool isUnknownZone = ( newPosix == "UTC0" &&
                                   ianaName != "" &&
                                   ianaName.indexOf( "UTC" ) < 0 &&
                                   ianaName.indexOf( "GMT" ) < 0 );

            if ( isUnknownZone ) {
                // POSIX string has OPPOSITE sign to UTC offset
                // UTC+7 (offset=+25200) → POSIX "UTC-7"
                // UTC-7 (offset=-25200) → POSIX "UTC7"
                int posixOffsetHours = -( currentOffset / 3600 );
                int posixOffsetMins  = abs( ( currentOffset % 3600 ) / 60 );
                if ( posixOffsetMins == 0 ) {
                    newPosix = "UTC" + String( posixOffsetHours );
                }
                else {
                    newPosix = "UTC" + String( posixOffsetHours ) + ":" + String( posixOffsetMins < 10 ? "0" : "" ) + String( posixOffsetMins );
                }
                if ( hasDst ) {
                    log_d( "[TZ-AUTO] DST zone without full POSIX rules, using current offset. Will auto-correct on next weather sync." );
                }
            }

            posixTZ = newPosix;
            log_i( "[TZ-AUTO] POSIX TZ set to: %s", posixTZ.c_str() );
            http.end();
            return;

        }
        else {
            log_e( "[TZ-AUTO] JSON Error: %s", error.c_str() );
        }
    }
    else {
        log_w( "[TZ-AUTO] HTTP Error: %d", httpCode );
    }
    http.end();

    // Fallback if timeapi.io fails
    log_w( "[TZ-AUTO] timeapi.io failed, using basic fallback" );
    if ( countryHint == "United Kingdom" || countryHint == "Ireland" || countryHint == "Portugal" ) {
        lookupTimezone = "Europe/London";
        lookupGmtOffset = 0;
        lookupDstOffset = 3600;
        posixTZ = "GMT0BST,M3.5.0/1,M10.5.0";
    }
    else if ( countryHint == "China" ) {
        lookupTimezone = "Asia/Shanghai";
        lookupGmtOffset = 28800;
        lookupDstOffset = 0;
        posixTZ = "CST-8";
    }
    else if ( countryHint == "Japan" ) {
        lookupTimezone = "Asia/Tokyo";
        lookupGmtOffset = 32400;
        lookupDstOffset = 0;
        posixTZ = "JST-9";
    }
    else if ( countryHint.indexOf( "America" ) >= 0 || countryHint == "United States" || countryHint == "Canada" ) {
        lookupTimezone = "America/New_York";
        lookupGmtOffset = -18000;
        lookupDstOffset = 3600;
        posixTZ = "EST5EDT,M3.2.0,M11.1.0";
    }
    else {
        lookupTimezone = "Europe/Prague";
        lookupGmtOffset = 3600;
        lookupDstOffset = 3600;
        posixTZ = "CET-1CEST,M3.5.0,M10.5.0/3";
    }
}
