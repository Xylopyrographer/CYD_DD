# Release Notes

---

## v1.0.2 — 2026-02-25

### Bug Fixes

- **Regional settings Back button** — Tapping Back in the Regional settings screen returned directly to the main clock face instead of the Settings menu. Fixed to match the behaviour of all other settings sub-screens.

- **Spurious NVS log errors on fresh/erased device** — On a device with no prior NVS data, startup generated a cascade of `[E] nvs_open NOT_FOUND` and `getString len fail` errors. Root cause: the read-only `prefs.begin()` probe fails with `nvs_open NOT_FOUND` when the namespace has never been written; subsequent key reads each log their own error. Fixed with a read-write probe open (which silently creates the empty namespace) followed by a single `isKey("ssid")` check; all NVS reads and location/recent-city loads are skipped if the device is uninitialised.

---

## v1.0.1 — 2026-02-25

### Bug Fixes

- **WiFi connect failure for manually-typed SSID** — Connecting to an SSID entered via the *Other…* option in the WiFi selection screen would always fail. Root cause: `WiFi.scanNetworks()` leaves an internal scan buffer allocated; when `WiFi.begin()` is subsequently called for an SSID that was not in the scan results, the ESP32 IDF WiFi stack's stale scan state blocks the targeted probe. Fixed by calling `WiFi.scanDelete()` immediately after the scan list is populated and again as a guard before every `WiFi.begin()` call.

### Code Quality

- **ArduinoJson v7 deprecations resolved** — Replaced `StaticJsonDocument<N>` and `DynamicJsonDocument(N)` with the unified `JsonDocument` type; replaced `.containsKey(k)` with a direct truthy subscript check as required by ArduinoJson v7 API. Affected files: `src/net/weather_api.cpp`, `src/net/ota.cpp`, `src/net/timezone.cpp`, `src/app/location.cpp`, `src/net/location.cpp`.

---

## v1.0.0 — 2026-02-25

Initial public release.

See [README-Data Display CYD Refactor.md](README-Data%20Display%20CYD%20Refactor.md) for a full summary of changes relative to the original source.

