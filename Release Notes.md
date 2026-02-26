# Release Notes

---

## v1.0.5 — 2026-02-26

### New Features

- **Public holiday display** — The clock face now shows today's public holiday name at the bottom of the date line (below the date/week row). Holidays are fetched from the [Nager.Date](https://date.nager.at) public API (free, no API key required) using a two-step HTTPS sequence: a fast yes/no check followed by the full holiday list for the year. The holiday name is shown in red (dark / blue themes) or dark-green (yellow / white themes). If no holiday applies the line is blank. Country ISO code is resolved automatically from the selected country and cached in NVS so the REST lookup is only performed once.

- **WiFi "Other…" always visible** — The *Other…* option in the WiFi selection screen is now permanently pinned at the bottom of the visible list rather than appearing only after scrolling to the end of the network list. Up to five scanned networks are shown in the scrollable area above it; the down arrow only appears when more than five networks were found.

- **Consistent action-item colour** — *Other…* (WiFi selection) and *Custom lookup* (country selection) are both rendered in blue to visually distinguish them from regular list entries and signal their shared role as keyboard-entry shortcuts.

### Build & Tooling

- **`[env:clean]` build environment** — A third PlatformIO environment (`[env:clean]`) is now available alongside `release` and `debug`. It sets `CORE_DEBUG_LEVEL=0` (all log macros compiled out) with `-Os` optimisation, producing the smallest possible binary (~62% Flash vs ~65% for `release`). Useful for checking available headroom before adding large features.

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

