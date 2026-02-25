#pragma once

#include <Arduino.h>

// Maps a known IANA timezone name to the equivalent POSIX TZ string.
// Returns "UTC0" for unrecognised zones (caller must treat that as "unknown").
String ianaToPostfixTZ( String iana );

// Detects the timezone for the given coordinates via timeapi.io.
// Side-effects (writes globals owned by main.cpp):
//   lookupTimezone, lookupGmtOffset, lookupDstOffset, posixTZ
void detectTimezoneFromCoords( float lat, float lon, String countryHint );
