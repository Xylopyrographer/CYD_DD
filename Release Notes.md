# Release Notes

---

## v1.0.6 — 2026-02-27

### New Features

- **Loading screen** — A *"Loading information. One moment…"* screen is displayed immediately after WiFi connects and persists while NTP syncs, the timezone is resolved, and the initial weather data is fetched. The clock layout then appears in a single clean render at full brightness with no intermediate blank frame or partial-draw flash.

- **DST toggle (MANUAL timezone mode)** — A *DST: OFF / DST: ON* button has been added to the Regional Setup screen when the device is in MANUAL timezone mode. Tapping it adjusts the active UTC offset by ±1 hour and persists the state across reboots. The toggle resets automatically when a city is selected from the built-in list, since city-derived timezone rules handle DST automatically.

- **SYNC modal feedback** — Tapping *SYNC* on the Regional Setup screen now shows an overlay dialog. *"Syncing…"* is displayed during the network request; on success *"Sync complete!"* is shown briefly before the overlay clears; on failure the error reason is shown with an *OK* button to dismiss. The device remains on the Regional Setup screen after a sync rather than navigating away automatically.

- **Settings inactivity timeout** — Any settings screen that receives no touch input for 3 minutes automatically returns to the main clock face.

- **Auto-dim level snap-to-grid** — The *+* and *−* buttons for the auto-dim brightness level now snap to the nearest 5% step. The *+* button also caps at the current normal brightness so the dim level can never be set above the screen's normal operating brightness.

### Bug Fixes

- **Tap-to-restore when auto-dimmed** — Tapping the display while it is in the auto-dimmed state now immediately restores normal brightness before processing the tap. Previously the first tap was consumed waking the display without producing any UI response.

- **Minimum brightness floor** — The brightness slider lower bound has been raised from 0 to ~12% (raw value 30). A preference saved at a near-zero value from a previous firmware version is clamped on boot, preventing the display from becoming inaccessibly dark.

- **Auto-dim level capped at normal brightness** — The auto-dim level can no longer be set above the screen's current normal brightness. Previously it was possible to configure a dim level that would *increase* brightness during auto-dim periods.

- **Graphics screen flicker** — Tapping any single-value control on the Graphics settings screen (clock style, auto-dim toggle, start/end/level, brightness slider) no longer triggers a full-screen repaint. Each control now redraws only its own region in place.

- **DST toggle and sync overlay flicker** — Tapping the DST button and dismissing the sync overlay previously caused a full-screen flash. Both now use targeted partial redraws.

- **Auto Dim section overlap** — The auto-dim section redraw rect was incorrectly sized, overwriting the NRM/FLP orientation widget and the back button. The clear region has been corrected.

- **Loading screen flicker** — The loading screen previously flashed blank while waiting for HTTP responses. The screen fill now happens before the network calls so the display remains stable throughout.

### Code Quality

- **Dead code removal** — Removed several functions that were defined but never called, reducing binary size.
- **Monitor time filter** — Serial monitor output is now prefixed with a timestamp filter for easier log reading during development.

---

## v1.0.5 — 2026-02-26

### New Features

- **Public holiday display** — The clock face now shows today's public holiday name beneath the date (in place of the Czech nameday when a holiday applies). Holidays are fetched from the free [Nager.Date](https://date.nager.at) API using the country selected in Regional settings. A two-step sequence minimises bandwidth: a quick yes/no check runs first; the full holiday list is only fetched when today is confirmed as a holiday. Regional (non-global) holidays are excluded. The ISO 3166-1 country code is resolved automatically from the selected country and cached in NVS so the lookup runs only once per device.

- **"Other…" always visible in WiFi scan list** — The *Other…* option for entering a WiFi network name manually is now permanently pinned at the bottom of the WiFi selection screen. Previously it appeared only after scrolling past all scanned networks; it is now always visible regardless of scroll position.

- **Consistent action-item colour** — Both *Other…* (WiFi list) and *Custom lookup* (country selection) are now rendered in blue to visually distinguish them from regular list entries and indicate that they open a free-text keyboard entry flow.

### Build & Tooling

- **`[env:clean]` build environment** — A third PlatformIO build environment has been added to `platformio.ini`. `[env:clean]` compiles with `CORE_DEBUG_LEVEL=0` (all log macros removed) and `-Os`, producing the smallest possible binary — useful for checking available flash headroom.

---

## v1.0.4 — 2026-02-25

### New Features

- **180° display rotation** — A new *Display Orientation* toggle (NRM / FLP) has been added to the Graphics settings screen. Selecting FLP rotates the display 180° so the device can be mounted upside-down; selecting NRM returns it to the normal orientation. The chosen orientation is persisted across reboots.

- **Per-orientation touch calibration** — Touch calibration is stored independently for each orientation. Separate NVS key-sets (`calXMin` / `calXMinF` etc.) hold normal and flipped calibration data with sensible uncalibrated defaults, so touch is accurate immediately after a flip without requiring a recalibration run. Running *Calibrate Touch* while in either orientation saves only the active orientation's calibration, leaving the other untouched.

---

## v1.0.3 — 2026-02-25

### Bug Fixes

- **Digital clock leading zero** — In 12-hour mode the hour was always zero-padded (e.g. *08:45*). The leading zero is now suppressed in 12-hour mode (e.g. *8:45*) and retained in 24-hour mode (e.g. *08:45*). The time string is recentred automatically each redraw so no layout shift occurs.

- **Spurious `NOT_FOUND` log error for recent cities** — On a device with no saved recent-city history the startup sequence logged `[E] recent0c NOT_FOUND` for every slot in the list. Root cause: `getString()` was called unconditionally and the NVS driver logs an error when the key does not exist. Fixed by probing with `isKey()` first and exiting the load loop early when the key is absent.

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

