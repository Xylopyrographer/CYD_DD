#pragma once
#include <Arduino.h>

// ================= HOLIDAYS MODULE =================
// Public holiday lookup via the Nager.Date API (date.nager.at).
// Covers 100+ countries identified by ISO 3166-1 alpha-2 code.
//
// Strategy:
//   1. Call IsTodayPublicHoliday/{cc}?offset={utcHours}
//      → 200 = holiday today, 204 = not a holiday, 404 = unsupported country
//   2. Only if 200: fetch PublicHolidays/{year}/{cc} and locate today's
//      localName from the array.
//   3. Cache the result in todayHoliday for the rest of the day.
//
// Works alongside the Czech static nameday table — both can display
// simultaneously for CZ (nameday every day + holiday on ~13 days/year).

// ── State variables (defined in holidays.cpp) ─────────────────────────────
extern String todayHoliday;    // localName for today's holiday, or ""
extern int    lastHolidayDay;  // tm_mday of the last check (-1 = never)
extern bool   holidayValid;    // true when todayHoliday holds a real name

// ── Functions ──────────────────────────────────────────────────────────────

// Fetch today's public holiday name for the given ISO country code and UTC
// offset (integer hours, -12..+12).  Returns the localName string, or ""
// if today is not a holiday or the country is unsupported.
// Performs up to two HTTPS requests; call from a WiFi-connected context only.
String fetchTodayHoliday( const String &isoCode, int utcOffsetHours );

// Call once per loop iteration (or on demand) to refresh todayHoliday.
// Uses lookupISOCode (set by lookupCountryEmbedded/lookupCountryRESTAPI) → fetchTodayHoliday().
// Updates once per day; sets forceClockRedraw = true when value changes.
void handleHolidayUpdate();
