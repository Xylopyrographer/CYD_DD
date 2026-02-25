# Data Display CYD Refactor 

Describes major changes made to the code base against DataDisplayCYD.ino version 1.4.1.

Date: 2026-02-25

---

## 1. Bug Fixes

| Fix                                                              | Detail |
|:------------------------------------------------------------------|:--------|
| **Startup white flash** | `pinMode(TFT_BL, OUTPUT)` + `digitalWrite(TFT_BL, LOW)` as first instructions in `setup()`. Residual ~1 s flash during boot ROM is a hardware issue (no pull-down on GPIO 21 gate); a 10 kΩ pull-down to GND fully eliminates it. |
| **Startup partial render** | Backlight held at 0 during first loop pass; revealed atomically after weather fetch + full clock render complete. No more partially-drawn clock before weather data loads. |
| **Clock hand glitch** | `updateHands()` replaced with sprite-based render. 140×140 px `TFT_eSprite` renders entire clock face off-screen each second; pushed with single `pushSprite()`. Eliminates partial-draw flicker. |
| **WiFi indicator clipped** | Sprite right edge is x=299. WiFi indicator moved to `fillCircle(305, 20, 4)` — 2 px clearance. Indicators redrawn after `pushSprite()`. |
| **Brightness slider flicker** | Slider drag formerly triggered full `fillScreen` redraw. Now calls `redrawBrightnessSlider()` — only fill bar and label repainted in-place. NVS write throttled to ≤1 per 500 ms. |
| **Brightness label artifact** | `setTextColor(fg, bg)` used for glyph background fill (eliminates artifact from mis-sized manual `fillRect`). |
| **Auto-dim brightness restore** | `applyAutoDim()` hardcoded `brightness = 255` on dim-off. Fixed with `static int preDimBrightness` capture before dim, restore on exit. |
| **+/− button alignment** | Auto-dim `+`/`−` buttons replaced `drawString` (off-centre font metrics) with `drawFastHLine`/`drawFastVLine` pixel lines centred at `(rx + btnW/2, ry + btnH/2)`. |
| **Backlight PWM** | `analogWrite()` not supported on ESP32 Arduino 3.x. Replaced with LEDC API: `ledcAttach(TFT_BL, 5000, 8)` + `ledcWrite()`. 8-bit (0–255) matches NVS `"bright"` range directly. |
| **Sunrise/sunset icons swapped** | `icon_sunrise[]` bytes form a sunset glyph and vice versa (misnamed in original). Fixed by swapping the array byte data so names match their visual. |
| **WiFi scan blank rows** | `scanWifiNetworks()` skipped entries where `WiFi.SSID(i)` is empty (hidden networks showing as blank rows). |
| **Invert toggle** | Display inversion logic was inverted. Fixed: `invertColors` maps directly to `tft.invertDisplay(invertColors)`. |
| **Nameday for non-Czech regions** | Nameday was showing regardless of selected country. Now suppressed unless country == Czech Republic. |
| **Down arrow scroll guard** | Scroll guard didn't account for the "Other…" row, allowing over-scroll. Fixed. |

---

## 2. New Features

| Feature                                             | Detail |
|:-----------------------------------------------------|:--------|
| **Touch calibration** | 2-point procedure in Settings → Calibrate (orange). Taps two crosshair targets; reads raw XPT2046 ADC values; extrapolates `touchXMin/Max/YMin/Max`. Saved to NVS; applied immediately without reboot. |
| **WiFi password obfuscation** | `obfuscatePassword()` / `deobfuscatePassword()` apply cycling 16-byte XOR then hex-encode before NVS write. Not cryptographic — prevents casual plaintext exposure. Backward-compatible with legacy plaintext values. |
| **Manual SSID entry** | "Other…" row in WiFi scan list opens on-screen keyboard for manual SSID entry. |

---

## 3. Build & Tooling

The project requires as the build system VS Code with the pioarduino extension installed.

At the time of writing, arduino-esp32 core version 3.3.2 was used.

Test platform was an original "single micro-USB port" CYD version. Modifications may be needed to accommodate the display used on other CYD variants.

|                   Change                                                  | Detail |
|:---------------------------------------------------------------------------|:--------|
| **Dual build environments** | `platformio.ini` split into shared `[env]` base + `[env:release]` + `[env:debug]`. One-line switch between production flash and verbose debug. |
| **Release log level** | `CORE_DEBUG_LEVEL=3` in release — `log_i/w/e` milestone output visible, `log_d` compiled out, no framework debug noise. |
| **Debug log level** | `CORE_DEBUG_LEVEL=4` in debug — all levels including ESP-IDF/HTTPClient internals. |
| **Binary export targets** | `scripts/custom_targets.py` registers `merged` and `ota` PlatformIO targets. `merged` stitches bootloader + partitions + boot_app0 + app into one flashable binary via esptool. `scripts/build_release.sh` runs clean → merged → ota in one shot. Outputs to `bin/` (gitignored). |

> Note: As a non Czech speaking person, comments were translated to English to facilitate better understanding of the code.

---

## 4. Code Quality

| Change                                                        | Detail |
|---------------------------------------------------------------|---------|
| **Serial → log_d()** | ~190 `Serial.print/println/printf` calls across 11 files replaced with `log_d()`. Zero overhead in production (compiled out at level < 4). |
| **Structured log levels** | All `log_d()` calls audited. Milestones → `log_i()`; degraded states → `log_w()`; hard failures → `log_e()`; step detail stays `log_d()`. |
| **Magic numbers → constants** | Inline numeric literals extracted to `src/util/constants.h`: HTTP timeouts, WiFi/weather intervals, `TOUCH_DEBOUNCE_MS`, `UI_DEBOUNCE_MS`, `UI_SPLASH_DELAY_MS`, `OTA_COUNTDOWN_SECS`, `THEME_DARK/WHITE/BLUE/YELLOW`. |
| **LVGL removed** | `src/lv_conf_source.h` (993 lines) and all LVGL artefacts removed. All UI is direct TFT_eSPI primitives. |
| **Czech comments translated** | All source comments translated from Czech to English. |
| **Includes consolidated** | All `#include` directives moved to top of `main.cpp`; orphan comment stubs from extraction removed. |

---

## 5. Architecture Refactor

`main.cpp` reduced from **~5,425 lines → 567 lines** (globals + `setup()` + `loop()` only) across six phases:


| Item |           Extracted to        |  Content            |
|:------------:|-------------------------------|---------------------|
| 1            | `hal/led.h`, `data/city_data.h`, `data/nameday.h/.cpp`, `data/app_state.h`, `util/moon.h/.cpp`, `util/string_utils.h/.cpp`, `util/constants.h` | LED defines, city/country tables, nameday logic, screen/data types, moon phase, string helpers, HTTP constants |
| 2            | `net/timezone.h/.cpp`, `net/ota.h/.cpp`, `net/weather_api.h/.cpp` | IANA→POSIX mapping, timeapi.io lookup, OTA version check + flash, geocoding + Open-Meteo fetch |
| 3            | `ui/theme.h/.cpp`, `ui/icons.h/.cpp`, `ui/clock_face.h/.cpp`, `ui/screens.h/.cpp` | Theme colour helpers, all vector drawing, clock/date/weather rendering, all draw*Screen functions + keyboard |
| 4           | `ui/touch_handler.h/.cpp` | Entire 1,179-line `switch(currentState)` dispatch extracted from `loop()` |
| 5           | `util/credentials.h/.cpp`, `data/recent.h/.cpp`, `net/location.h/.cpp` | Password obfuscation, recent cities NVS, country/city REST + embedded + Nominatim lookups |
| 6          | `app/location.h/.cpp`, `hal/backlight.h/.cpp` | syncRegion/applyLocation/loadSavedLocation, applyAutoDim |

**Dependency direction**: `main` → `app/ui` → `net` → `util/data` (no upward dependencies).



<!-- //  --- EOF --- // -->
