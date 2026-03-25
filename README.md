# Cheap Yellow Display Data Display
#### (a.k.a. CYD_DD)

A "desk ornament" using the infamous Cheap Yellow Display to show time, date, moon phase, special days and weather information.

## Installing

To install a pre-built binary, click the "Latest" button by "Releases" and scroll down to "Assets".

Download the file ending in `..."FULL.bin"` and flash that to your CYD at address 0x0000.

To build your own, the development environment is VSCode using the `pioarduino` extension.

## First Time Setup

After installing, the device will detect it is a first-time run and show the WiFi setup screen. The device will scan available networks. Scroll and tap to select your network and then enter the password.

Or, tap "Other" and enter the SSID and password of the network.

When connected, the green dot at the top right of the display will be shown as confirmation.

## Options

Tapping the gear icon at the bottom right will take you to a set of menus for setting various options. They are pretty self explanatory so explore away.

All settings are retained across power up and restarts of the device.

## Digital Clock Screen

When using the digital clock face:
- Tapping the `HH:MM` toggles between 24 and 12 hour display.
- When in 12 hour display, AM and PM is indicated by an orange dot to the right of the minutes; top for AM, bottom for PM.
- Tapping the seconds area toggles showing or hiding seconds.

## Updates

Availability of a firmware update is indicated by an "up arrow" icon to the right of the WiFi connected dot at the top right.

When shown, tapping the arrow will take your directly to the FIRMWARE options screen as a shortcut. 

## Acknowledgement

This project is based on the project at https://github.com/lachimalaif/DataDisplay-V1-instalator project. It has been completely refactored for maintainability and ease of feature addition.

---

# Release Notes

## v2.1.0 — 2026-03-25

### New Features

- **Digital clock seconds toggle** — Tapping the seconds area of the digital clock face now toggles whether seconds are shown or hidden. The preference is preserved across reboots.

- **Digital clock seconds font** — The font used for the seconds digits has been replaced with one that matches the style of the `HH:MM` display.

## v2.0.0 — 2026-03-11

### New Features

- **Digital clock AM/PM indicator** — In 12-hour mode, a small orange dot is shown to the right of the `HH:MM` digits: top position = AM, bottom position = PM. 

- **Digital clock seconds display** — The seconds colon separator has been removed.

### Bug Fixes

- **4-digit to 3-digit time transition bug fix** — In 12-hour mode, switching from a 4-digit time (e.g. `12:59`) to a 3-digit time (e.g. `1:00`) previously left pixel artifacts on the right. Fixed.

### Improvements

- **Humidity / pressure label spacing** — The weather line now reads `RH: 65%  P: 1013 hPa` to leave more room between the time display.

### Build & Tooling

- **Version numbering harmonized** — Firmware and project release versions are now aligned. Prior `1.4.x` firmware numbers were inherited from the upstream project; this release establishes `2.0.0` as the baseline for this fork, which now has its own OTA update channel at `https://github.com/Xylopyrographer/CYD_DD`.

## v1.0.7 — 2026-02-27

### New Features

- **Update indicator tap shortcut** — Tapping anywhere over the WiFi indicator or update icon (top-right corner) when a firmware update is available navigates directly to the Firmware settings screen. The standard route through Settings → Firmware remains available. The touch zone is inert when no update is available.
- **OTA version check pointing to own repository** — `VERSION_CHECK_URL` now resolves against the project's own `version.json` at the root of this repository, replacing the previous upstream URL. This decouples the project's OTA release cadence from the upstream author's repository.

### Bug Fixes

- **Settings menu hit-test bounds** — Tapping in the empty space to the left or right of a Settings menu item no longer incorrectly activates the nearest item.
- **OTA progress bar flicker** — During firmware download and install the entire display region was cleared on every progress tick, causing a visible flicker. Fixed.
- **Update indicator ghost artifacts** — The update icon left residual pixels on theme changes and forced redraws. Fixed.

## v1.0.6 — 2026-02-27

### New Features

- **Loading screen** — A *"Loading information. One moment…"* screen is displayed immediately after WiFi connects and persists while NTP syncs, the timezone is resolved, and the initial weather data is fetched. The clock layout then appears in a single clean render at full brightness with no intermediate blank frame or partial-draw flash.
- **DST toggle (MANUAL timezone mode)** — A *DST: OFF / DST: ON* button has been added to the Regional Setup screen when the device is in MANUAL timezone mode. Tapping it adjusts the active UTC offset by ±1 hour and persists the state across reboots. The toggle resets automatically when a city is selected from the built-in list, since city-derived timezone rules handle DST automatically.
- **SYNC modal feedback** — Tapping *SYNC* on the Regional Setup screen now shows an overlay dialog. *"Syncing…"* is displayed during the network request; on success *"Sync complete!"* is shown briefly before the overlay clears; on failure the error reason is shown with an *OK* button to dismiss. The device remains on the Regional Setup screen after a sync rather than navigating away automatically.
- **Settings inactivity timeout** — Any settings screen that receives no touch input for 3 minutes automatically returns to the main clock face.
- **Auto-dim level snap-to-grid** — The *+* and *−* buttons for the auto-dim brightness level now snap to the nearest 5% step. The *+* button also caps at the current normal brightness so the dim level can never be set above the screen's normal operating brightness.

### Bug Fixes

- **Tap-to-restore when auto-dimmed** — Tapping the display while it is in the auto-dimmed state now immediately restores normal brightness before processing the tap.
- **Minimum brightness floor** — The brightness slider lower bound has been raised from 0 to ~12% (raw value 30). A preference saved at a near-zero value from a previous firmware version is clamped on boot, preventing the display from becoming inaccessibly dark.
- **Auto-dim level capped at normal brightness** — The auto-dim level can no longer be set above the screen's current normal brightness. Previously it was possible to configure a dim level that would *increase* brightness during auto-dim periods.
- **Graphics screen flicker** — Tapping any single-value control on the Graphics settings screen (clock style, auto-dim toggle, start/end/level, brightness slider) no longer triggers a full-screen repaint. Each control now redraws only its own region in place.
- **DST toggle and sync overlay flicker** — Tapping the DST button and dismissing the sync overlay previously caused a full-screen flash. Fixed.
- **Auto Dim section overlap** — The auto-dim section redraw rect was incorrectly sized, overwriting the NRM/FLP orientation widget and the back button. Fixed.
- **Loading screen flicker** — The loading screen previously flashed blank while waiting for HTTP responses. The screen fill now happens before the network calls so the display remains stable throughout.

## v1.0.5 — 2026-02-26

### New Features

- **Public holiday display** — The clock face now shows today's public holiday name beneath the date (in place of the Czech nameday when a holiday applies). Holidays are fetched from the free [Nager.Date](https://date.nager.at) API using the country selected in Regional settings. A two-step sequence minimises bandwidth: a quick yes/no check runs first; the full holiday list is only fetched when today is confirmed as a holiday. Regional (non-global) holidays are excluded. The ISO 3166-1 country code is resolved automatically from the selected country and cached in NVS so the lookup runs only once per device.

- **"Other…" always visible in WiFi scan list** — The *Other…* option for entering a WiFi network name manually is now permanently pinned at the bottom of the WiFi selection screen. Previously it appeared only after scrolling past all scanned networks; it is now always visible regardless of scroll position.

- **Consistent action-item colour** — Both *Other…* (WiFi list) and *Custom lookup* (country selection) are now rendered in blue to visually distinguish them from regular list entries and indicate that they open a free-text keyboard entry flow.

## v1.0.4 — 2026-02-25

### New Features

- **180° display rotation** — A new *Display Orientation* toggle (NRM / FLP) has been added to the Graphics settings screen. Selecting FLP rotates the display 180° so the device can be mounted upside-down; selecting NRM returns it to the normal orientation. The chosen orientation is persisted across reboots.

- **Per-orientation touch calibration** — Touch calibration is stored independently for each orientation. Separate NVS key-sets (`calXMin` / `calXMinF` etc.) hold normal and flipped calibration data with sensible uncalibrated defaults, so touch is accurate immediately after a flip without requiring a recalibration run. Running *Calibrate Touch* while in either orientation saves only the active orientation's calibration, leaving the other untouched.

## v1.0.3 — 2026-02-25

### Bug Fixes

- **Digital clock leading zero** — In 12-hour mode the hour was always zero-padded (e.g. *08:45*). The leading zero is now suppressed in 12-hour mode (e.g. *8:45*) and retained in 24-hour mode (e.g. *08:45*). The time string is recentred automatically each redraw so no layout shift occurs.

- **Spurious `NOT_FOUND` log error for recent cities** — On a device with no saved recent-city history the startup sequence logged `[E] recent0c NOT_FOUND` for every slot in the list. Fixed.

## v1.0.2 — 2026-02-25

### Bug Fixes

- **Regional settings Back button** — Tapping Back in the Regional settings screen returned directly to the main clock face instead of the Settings menu. Fixed to match the behaviour of all other settings sub-screens.

- **Spurious NVS log errors on fresh/erased device** — On a device with no prior NVS data, startup generated a cascade of `[E] nvs_open NOT_FOUND` and `getString len fail` errors. Fixed.

## v1.0.1 — 2026-02-25

### Bug Fixes

- **WiFi connect failure for manually-typed SSID** — Connecting to an SSID entered via the *Other…* option in the WiFi selection screen would always fail. Fixed.

## v1.0.0 — 2026-02-25

Initial public release.

---


<!-- EOF -->
