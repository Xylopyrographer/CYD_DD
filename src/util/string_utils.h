#pragma once

#include <Arduino.h>

// Returns a copy of `input` with the first letter of each word capitalised.
String toTitleCase( String input );

// Returns true if `input` loosely matches `target` (case-insensitive prefix/contains).
bool fuzzyMatch( String input, String target );

// Replaces common Central- and Eastern-European diacritic characters with their
// ASCII equivalents (handles both lower- and upper-case).
String removeDiacritics( String input );

// Maps a country name (English) to its two-letter ISO 3166-1 alpha-2 code.
// Returns "US" for unrecognised names.

