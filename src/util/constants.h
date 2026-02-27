#pragma once

// ============================================================
// Shared timing and HTTP timeout constants
// All durations in milliseconds unless noted.
// ============================================================

// WiFi / connectivity
constexpr unsigned long WIFI_CONNECT_TIMEOUT    = 15000UL;   // Max wait for initial WiFi association
constexpr unsigned long WIFI_RECONNECT_INTERVAL = 30000UL;   // How often to retry a lost connection

// Weather refresh
constexpr unsigned long WEATHER_UPDATE_INTERVAL    = 1800000UL; // 30 min weather refresh
constexpr unsigned long BRIGHTNESS_UPDATE_INTERVAL =   60000UL; // How often to re-evaluate auto-dim

// Touch / UI interaction
constexpr int TOUCH_DEBOUNCE_MS  = 200;  // Minimum ms between touch events in main loop
constexpr int UI_DEBOUNCE_MS     = 150;  // Button-tap debounce delay after an action
constexpr int UI_SPLASH_DELAY_MS = 2000; // Duration of splash / status message displays

constexpr unsigned long SETTINGS_INACTIVITY_TIMEOUT = 180000UL; // 3 min — return to CLOCK if no touch while in any settings screen

// OTA
constexpr int OTA_COUNTDOWN_SECS = 10;   // Countdown seconds shown before OTA reboot/rollback

// HTTP timeouts (per request type)
constexpr int HTTP_TIMEOUT_SHORT     = 3000;   // Quick sub-request (e.g. coord lookup step 1)
constexpr int HTTP_TIMEOUT_STANDARD  = 5000;   // Standard REST calls (weather, WiFi setup)
constexpr int HTTP_TIMEOUT_GEO       = 8000;   // Country/city geocoding APIs
constexpr int HTTP_TIMEOUT_VERSION   = 10000;  // OTA version-check (version.json)
constexpr int HTTP_TIMEOUT_NOMINATIM = 12000;  // Nominatim (can be slow on first request)
constexpr int HTTP_TIMEOUT_OTA       = 30000;  // OTA firmware download stream

// Theme mode identifiers (stored in NVS as "themeMode")
constexpr int THEME_DARK   = 0;  // Classic dark background
constexpr int THEME_WHITE  = 1;  // Classic white background
constexpr int THEME_BLUE   = 2;  // Blue gradient
constexpr int THEME_YELLOW = 3;  // Yellow gradient
