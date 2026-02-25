#pragma once
#include <Arduino.h>

// ================= NAMEDAY MODULE =================
// Czech nameday lookup and daily update logic.
// Only active when selectedCountry == "Czech Republic".

// ── State variables (defined in nameday.cpp) ──────────────────────────────────
extern String todayNameday;   // Name to display, e.g. "Josef"
extern int    lastNamedayDay;  // tm_mday of the last update  (-1 = never)
extern int    lastNamedayHour; // tm_hour of the last update  (-1 = never)
extern bool   namedayValid;    // true when todayNameday is meaningful

// ── Functions ─────────────────────────────────────────────────────────────────

// Return the Czech nameday string for the given day/month (1-based).
// Returns "--" for invalid dates or days outside the table.
String getNamedayForDate( int day, int month );

// Call once per loop iteration (or on demand) to refresh todayNameday.
// Sets forceClockRedraw = true when the name changes.
void handleNamedayUpdate();
