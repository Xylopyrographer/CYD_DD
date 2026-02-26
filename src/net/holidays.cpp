#include "holidays.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

#include "../util/constants.h"
#include "../util/string_utils.h"

// ── Globals defined here ───────────────────────────────────────────────────
String todayHoliday   = "";
int    lastHolidayDay  = -1;
bool   holidayValid    = false;

// ── External globals owned by main.cpp ────────────────────────────────────
extern String selectedCountry;
extern int    lookupGmtOffset;   // UTC offset in seconds (from timezone detect)
extern bool   forceClockRedraw;

// ── Internal helpers ───────────────────────────────────────────────────────

// Build "YYYY-MM-DD" for today in local time.
static String todayDateString() {
    time_t     now      = time( nullptr );
    struct tm *timeinfo = localtime( &now );
    if ( !timeinfo ) {
        return "";
    }
    char buf[ 11 ];
    snprintf( buf, sizeof( buf ), "%04d-%02d-%02d",
              timeinfo->tm_year + 1900,
              timeinfo->tm_mon + 1,
              timeinfo->tm_mday );
    return String( buf );
}

// ── Public functions ───────────────────────────────────────────────────────

String fetchTodayHoliday( const String &isoCode, int utcOffsetHours ) {
    if ( isoCode.isEmpty() ) {
        log_d( "[HOLIDAY] No ISO code — skipping" );
        return "";
    }

    HTTPClient http;
    http.setTimeout( HTTP_TIMEOUT_SHORT );

    // ── Step 1: Is today a public holiday? ────────────────────────────────
    // Returns 200=yes, 204=no, 404=unsupported country
    String checkUrl = "https://date.nager.at/api/v3/IsTodayPublicHoliday/" + isoCode +
                      "?offset=" + String( utcOffsetHours );
    log_d( "[HOLIDAY] Checking: %s", checkUrl.c_str() );

    http.begin( checkUrl );
    int checkCode = http.GET();
    http.end();

    log_d( "[HOLIDAY] IsTodayPublicHoliday → %d", checkCode );

    if ( checkCode == 204 ) {
        return "";   // Not a holiday — normal day
    }
    if ( checkCode != 200 ) {
        log_w( "[HOLIDAY] Unexpected status %d for country %s", checkCode, isoCode.c_str() );
        return "";
    }

    // ── Step 2: Fetch the year list and find today's localName ────────────
    time_t     now      = time( nullptr );
    struct tm *timeinfo = localtime( &now );
    if ( !timeinfo ) {
        return "";
    }

    int    year    = timeinfo->tm_year + 1900;
    String today   = todayDateString();

    String listUrl = "https://date.nager.at/api/v3/PublicHolidays/" + String( year ) + "/" + isoCode;
    log_d( "[HOLIDAY] Fetching year list: %s", listUrl.c_str() );

    http.setTimeout( HTTP_TIMEOUT_STANDARD );
    http.begin( listUrl );
    int listCode = http.GET();

    if ( listCode != 200 ) {
        log_w( "[HOLIDAY] Year list HTTP %d", listCode );
        http.end();
        return "";
    }

    String payload = http.getString();
    http.end();

    // ── Step 3: Parse + find today's entry ────────────────────────────────
    // Prefer global=true entries; accept non-global as fallback
    JsonDocument doc;
    DeserializationError err = deserializeJson( doc, payload );
    if ( err ) {
        log_e( "[HOLIDAY] JSON error: %s", err.c_str() );
        return "";
    }

    String globalMatch   = "";
    String provincialMatch = "";

    for ( JsonVariant entry : doc.as<JsonArray>() ) {
        String entryDate = entry[ "date" ].as<String>();
        if ( entryDate != today ) {
            continue;
        }

        String name     = entry[ "localName" ].as<String>();
        bool   isGlobal = entry[ "global" ].as<bool>();

        if ( isGlobal && globalMatch.isEmpty() ) {
            globalMatch = name;
        }
        else if ( !isGlobal && provincialMatch.isEmpty() ) {
            provincialMatch = name;
        }
    }

    String result = globalMatch.isEmpty() ? provincialMatch : globalMatch;

    if ( result.isEmpty() ) {
        // IsTodayPublicHoliday returned 200 but we couldn't find the entry —
        // shouldn't happen but handle gracefully.
        log_w( "[HOLIDAY] Holiday flagged but no matching date found in list" );
    }
    else {
        log_i( "[HOLIDAY] Today is: %s", result.c_str() );
    }

    return result;
}

void handleHolidayUpdate() {
    // Gate on WiFi and valid time
    if ( WiFi.status() != WL_CONNECTED ) {
        return;
    }

    time_t     now      = time( nullptr );
    struct tm *timeinfo = localtime( &now );
    if ( !timeinfo ) {
        return;
    }
    if ( timeinfo->tm_year < 125 ) {
        return;    // time not synced (<2025)
    }

    int today = timeinfo->tm_mday;
    if ( today == lastHolidayDay ) {
        return;    // Already checked today
    }

    lastHolidayDay = today;

    // ISO country code — empty string means unsupported / unknown
    String isoCode = countryToISO( selectedCountry );

    // UTC offset in whole hours
    int offsetHours = lookupGmtOffset / 3600;

    String name = fetchTodayHoliday( isoCode, offsetHours );

    if ( name != todayHoliday ) {
        todayHoliday = name;
        holidayValid = !name.isEmpty();
        forceClockRedraw = true;
        log_d( "[HOLIDAY] Updated: '%s' (valid=%d)", todayHoliday.c_str(), holidayValid );
    }
}
