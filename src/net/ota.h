#pragma once

#include <Arduino.h>

// Returns true if newVer is strictly newer than currentVer (X.Y.Z format, "v" prefix stripped).
bool isNewerVersion( String currentVer, String newVer );

// Checks GitHub version.json for an available update.
// Side-effects: sets updateAvailable, availableVersion, downloadURL, lastVersionCheck.
void checkForUpdate();

// Downloads and flashes firmware from downloadURL.
// Shows progress on TFT. Reboots on success, returns to firmware screen on failure.
void performOTAUpdate();
