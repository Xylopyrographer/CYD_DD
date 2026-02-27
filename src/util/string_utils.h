#pragma once

#include <Arduino.h>

// Returns a copy of `input` with the first letter of each word capitalised.
String toTitleCase( String input );

// Returns true if `input` loosely matches `target` (case-insensitive prefix/contains).
bool fuzzyMatch( String input, String target );

