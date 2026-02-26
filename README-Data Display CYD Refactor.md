# Data Display CYD Refactor 

Describes major changes made to the code base against DataDisplayCYD.ino version 1.4.1.

Date: 2026-02-25  
Last updated: 2026-02-26 (`calendar` branch)

---

## 1. Bug Fixes


<table>
<colgroup><col style="width:28%"><col style="width:72%"></colgroup>
<thead><tr><th>Fix</th><th>Detail</th></tr></thead>
<tbody>
<tr><td><strong>Startup white flash</strong></td><td><code>pinMode(TFT_BL, OUTPUT)</code> + <code>digitalWrite(TFT_BL, LOW)</code> as first instructions in <code>setup()</code>. Residual ~1 s flash during boot ROM is a hardware issue (no pull-down on GPIO 21 gate); a 10 kΩ pull-down to GND should eliminate it.</td></tr>
<tr><td><strong>Startup partial render</strong></td><td>Backlight held at 0 during first loop pass; revealed atomically after weather fetch + full clock render complete. No more partially-drawn clock before weather data loads.</td></tr>
<tr><td><strong>RGB LED on at startup</strong></td><td>The three discrete CYD LEDs (R/G/B, active-LOW) illuminate on every cold boot because the ESP32 GPIO pins reset to INPUT / float LOW, forward-biasing the LEDs. Fixed by <code>initLEDS()</code> in <code>hal/led.h</code>: sets all three pins to OUTPUT and drives them HIGH (<code>CYD_LED_RGB_OFF()</code>) as part of <code>setup()</code>. Convenience macros <code>CYD_LED_RED/GREEN/BLUE_ON/OFF()</code> and <code>CYD_LED_RGB_ON/OFF()</code> provided for explicit control if needed.</td></tr>
<tr><td><strong>Clock hand glitch</strong></td><td><code>updateHands()</code> replaced with sprite-based render. 140×140 px <code>TFT_eSprite</code> renders entire clock face off-screen each second; pushed with single <code>pushSprite()</code>. Eliminates partial-draw flicker.</td></tr>
<tr><td><strong>WiFi indicator clipped</strong></td><td>Sprite right edge is x=299. WiFi indicator moved to <code>fillCircle(305, 20, 4)</code> — 2 px clearance. Indicators redrawn after <code>pushSprite()</code>.</td></tr>
<tr><td><strong>Brightness slider flicker</strong></td><td>Slider drag formerly triggered full <code>fillScreen</code> redraw. Now calls <code>redrawBrightnessSlider()</code> — only fill bar and label repainted in-place. NVS write throttled to ≤1 per 500 ms.</td></tr>
<tr><td><strong>Brightness label artifact</strong></td><td><code>setTextColor(fg, bg)</code> used for glyph background fill (eliminates artifact from mis-sized manual <code>fillRect</code>).</td></tr>
<tr><td><strong>Auto-dim brightness restore</strong></td><td><code>applyAutoDim()</code> hardcoded <code>brightness = 255</code> on dim-off. Fixed with <code>static int preDimBrightness</code> capture before dim, restore on exit.</td></tr>
<tr><td><strong>+/− button alignment</strong></td><td>Auto-dim +/− buttons replaced <code>drawString</code> (off-centre font metrics) with <code>drawFastHLine</code>/<code>drawFastVLine</code> pixel lines centred at <code>(rx + btnW/2, ry + btnH/2)</code>.</td></tr>
<tr><td><strong>Backlight PWM</strong></td><td><code>analogWrite()</code> replaced with LEDC API: <code>ledcAttach(TFT_BL, 5000, 8)</code> + <code>ledcWrite()</code>. 8-bit (0–255) matches NVS <code>"bright"</code> range directly.</td></tr>
<tr><td><strong>Sunrise/sunset icons swapped</strong></td><td><code>icon_sunrise[]</code> bytes form a sunset glyph and vice versa. Fixed by swapping the array names so they match their visual.</td></tr>
<tr><td><strong>WiFi scan blank rows</strong></td><td><code>scanWifiNetworks()</code> skips entries where <code>WiFi.SSID(i)</code> is empty (hidden networks showing as blank rows).</td></tr>
<tr><td><strong>Invert toggle</strong></td><td>Display inversion logic was inverted. Fixed: <code>invertColors</code> maps directly to <code>tft.invertDisplay(invertColors)</code>.<br><strong>Note</strong>: May need reverting for displays used on some CYD variants.</td></tr>
<tr><td><strong>Nameday for non-Czech regions</strong></td><td>Nameday was showing regardless of selected country. Now suppressed unless country == Czech Republic.</td></tr>
<tr><td><strong>Down arrow scroll guard</strong></td><td>Scroll guard didn't account for the "Other…" row, allowing over-scroll. Fixed.</td></tr>
<tr><td><strong>WiFi connect failure (manual SSID)</strong></td><td>Connecting via the "Other…" option always failed. <code>WiFi.scanNetworks()</code> leaves a stale scan buffer that blocks <code>WiFi.begin()</code> for SSIDs not in the scan results. Fixed by calling <code>WiFi.scanDelete()</code> after the scan list is populated and again immediately before every <code>WiFi.begin()</code> call.</td></tr>
<tr><td><strong>NVS NOT_FOUND errors on fresh / erased device</strong></td><td>On a device with no prior NVS data, startup generated a cascade of <code>NOT_FOUND</code> and <code>getString len fail</code> log errors. Root cause: a read-only <code>prefs.begin()</code> probe fails when the namespace has never been written, and every subsequent key read logs its own error. Fixed with a read-write probe open (silently creates the empty namespace) followed by a single <code>isKey("ssid")</code> guard; all NVS reads are skipped if the device is uninitialised.</td></tr>
<tr><td><strong>Regional settings Back button</strong></td><td>Tapping Back in Regional settings returned to the clock face instead of the Settings menu. Fixed to match the behaviour of all other settings sub-screens.</td></tr>
<tr><td><strong>Digital clock leading zero (12-hour mode)</strong></td><td>Hour was always zero-padded (e.g. <code>08:45</code>). Leading zero now suppressed in 12-hour mode (<code>8:45</code>) and retained in 24-hour mode (<code>08:45</code>). Time string recentred automatically each redraw.</td></tr>
<tr><td><strong>Recent cities NOT_FOUND log errors</strong></td><td>On a device with no saved recent-city history, startup logged <code>NOT_FOUND</code> for every slot. Fixed by probing with <code>isKey()</code> first and exiting the load loop early when the key is absent.</td></tr>
</tbody>
</table>

---

## 2. New Features

<table>
<colgroup><col style="width:28%"><col style="width:72%"></colgroup>
<thead><tr><th>Feature</th><th>Detail</th></tr></thead>
<tbody>
<tr><td><strong>Touch calibration</strong></td><td>2-point procedure in Settings → Calibrate (orange). Taps two crosshair targets; reads raw XPT2046 ADC values; extrapolates <code>touchXMin/Max/YMin/Max</code>. Saved to NVS; applied immediately without reboot.</td></tr>
<tr><td><strong>WiFi password obfuscation</strong></td><td><code>obfuscatePassword()</code> / <code>deobfuscatePassword()</code> apply cycling 16-byte XOR then hex-encode before NVS write. Not cryptographic — prevents casual plaintext exposure. Backward-compatible with legacy plaintext values.</td></tr>
<tr><td><strong>Manual SSID entry</strong></td><td>"Other…" row in WiFi scan list opens on-screen keyboard for manual SSID entry.</td></tr>
<tr><td><strong>180° display rotation</strong></td><td>New <em>Display Orientation</em> toggle (NRM / FLP) in Graphics settings. FLP rotates the display 180° for upside-down mounting; NRM returns to normal. Chosen orientation persisted to NVS (<code>dispFlip</code>) and applied on every boot. <code>tft.setRotation(1|3)</code> is used; <code>ts.setRotation()</code> is always kept at 1 (the XPT2046 library ignores it — <code>getPoint()</code> returns raw ADC regardless).</td></tr>
<tr><td><strong>Per-orientation touch calibration</strong></td><td>Touch calibration stored independently per orientation using separate NVS keysets: <code>calXMin/Max/YMin/Max</code> for normal, <code>calXMinF/MaxF/YMinF/MaxF</code> for flipped. Flipped defaults (3900/200/3900/200) encode the reversed axis direction so touch is accurate immediately after a flip without requiring a calibration run. Calibrating in either orientation saves only that orientation's keyset. Literal defaults used in all NVS reads to prevent cross-orientation contamination on toggle.</td></tr>
<tr><td><strong>Public holiday display</strong></td><td>Fetches today's public holidays from <a href="https://date.nager.at">Nager.Date</a> (free, no API key) and displays the holiday name on the clock face beneath the date (red on dark/blue themes, dark-green on yellow/white). Two-step HTTPS sequence: (1) <code>IsTodayPublicHoliday/{cc}/{date}</code> confirmation check; (2) <code>PublicHolidays/{year}/{cc}</code> full list to retrieve the name. Country ISO code (<code>lookupISOCode</code>) resolved by matching the selected country against a built-in 11-country table or, for unlisted countries, via <code>restcountries.com</code> REST lookup; code persisted to NVS key <code>isoCode</code> so the REST call is only made once. Holiday entries with <code>global=false</code> are skipped. Priority ladder at display y=227: holiday (red) &gt; Czech nameday (orange) &gt; empty. Compile-time <code>HOLIDAY_TEST_DATE="YYYY-MM-DD"</code> flag bypasses the IsTodayPublicHoliday check and injects a fixed date for testing future holidays without flashing a production build.</td></tr>
</tbody>
</table>

---

## 3. Build & Tooling

The project requires as the build system VS Code with the pioarduino extension installed.

At the time of writing, arduino-esp32 core version 3.3.2 was used.

Test platform was an original "single micro-USB port" CYD version. Modifications may be needed to accommodate the display used on other CYD variants.

<table>
<colgroup><col style="width:28%"><col style="width:72%"></colgroup>
<thead><tr><th>Change</th><th>Detail</th></tr></thead>
<tbody>
<tr><td><strong>Dual build environments</strong></td><td><code>platformio.ini</code> split into shared <code>[env]</code> base + <code>[env:release]</code> + <code>[env:debug]</code>. One-line switch between production flash and verbose debug.</td></tr>
<tr><td><strong>Release log level</strong></td><td><code>CORE_DEBUG_LEVEL=3</code> in release — <code>log_i/w/e</code> milestone output visible, <code>log_d</code> compiled out, no framework debug noise.</td></tr>
<tr><td><strong>Debug log level</strong></td><td><code>CORE_DEBUG_LEVEL=4</code> in debug — all levels including ESP-IDF/HTTPClient internals.</td></tr>
<tr><td><strong>Clean build environment</strong></td><td><code>[env:clean]</code> added to <code>platformio.ini</code>: <code>CORE_DEBUG_LEVEL=0</code> (all log macros compiled out) + <code>-Os</code>. Produces the smallest possible binary — useful for checking headroom. Flash: <strong>62.3%</strong> vs 64.8% for <code>[env:release]</code>.</td></tr>
<tr><td><strong>Custom partition table</strong></td><td><code>partitions/DataDisplayCYD.csv</code> replaces the default partition layout: NVS 20 KB, OTA data 8 KB, <strong>app0 1984 KB</strong>, app1 1984 KB. Eliminates the default 1 MB SPIFFS partition, maximising application space for OTA. Both app0 and app1 are equal size — required for reliable OTA swap.</td></tr>
<tr><td><strong>Binary export targets</strong></td><td><code>scripts/custom_targets.py</code> registers <code>merged</code> and <code>ota</code> PlatformIO targets. <code>merged</code> stitches bootloader + partitions + boot_app0 + app into one flashable binary via esptool. <code>scripts/build_release.sh</code> runs clean → merged → ota in one shot. Outputs to <code>bin/</code> (gitignored).</td></tr>
</tbody>
</table>

---

## 4. Code Quality

<table>
<colgroup><col style="width:28%"><col style="width:72%"></colgroup>
<thead><tr><th>Change</th><th>Detail</th></tr></thead>
<tbody>
<tr><td><strong>Serial → log_d()</strong></td><td>~190 <code>Serial.print/println/printf</code> calls across 11 files replaced with <code>log_d()</code>. Zero overhead in production (compiled out at level &lt; 4).</td></tr>
<tr><td><strong>Structured log levels</strong></td><td>All <code>log_d()</code> calls audited. Milestones → <code>log_i()</code>; degraded states → <code>log_w()</code>; hard failures → <code>log_e()</code>; step detail stays <code>log_d()</code>.</td></tr>
<tr><td><strong>Magic numbers → constants</strong></td><td>Inline numeric literals extracted to <code>src/util/constants.h</code>: HTTP timeouts, WiFi/weather intervals, <code>TOUCH_DEBOUNCE_MS</code>, <code>UI_DEBOUNCE_MS</code>, <code>UI_SPLASH_DELAY_MS</code>, <code>OTA_COUNTDOWN_SECS</code>, <code>THEME_DARK/WHITE/BLUE/YELLOW</code>.</td></tr>
<tr><td><strong>LVGL removed</strong></td><td><code>src/lv_conf_source.h</code> (993 lines) and all LVGL artefacts removed. All UI is direct TFT_eSPI primitives.</td></tr>
<tr><td><strong>Czech comments translated</strong></td><td>Not a code quality issue per se, but as a non Czech speaking person, comments were translated to English to facilitate better understanding of the code.</td></tr>
<tr><td><strong>Includes consolidated</strong></td><td>All <code>#include</code> directives moved to top of <code>main.cpp</code>; orphan comment stubs from extraction removed.</td></tr>
<tr><td><strong>ArduinoJson v7 deprecations</strong></td><td>Replaced <code>StaticJsonDocument&lt;N&gt;</code> and <code>DynamicJsonDocument(N)</code> with the unified <code>JsonDocument</code> type; replaced <code>.containsKey(k)</code> with a direct truthy subscript check. Affected files: <code>weather_api.cpp</code>, <code>ota.cpp</code>, <code>timezone.cpp</code>, <code>location.cpp</code> (app + net).</td></tr>
</tbody>
</table>


---

## 5. Architecture Refactor

`main.cpp` reduced from **~5,425 lines → 567 lines** (globals + `setup()` + `loop()` only) across six phases:


<table>
<colgroup><col style="width:3.5em"><col style="width:40%"><col></colgroup>
<thead><tr><th>Itm</th><th>Extracted to</th><th>Content</th></tr></thead>
<tbody>
<tr><td>1</td><td><code>hal/led.h</code>, <code>data/city_data.h</code>, <code>data/nameday.h/.cpp</code>, <code>data/app_state.h</code>, <code>util/moon.h/.cpp</code>, <code>util/string_utils.h/.cpp</code>, <code>util/constants.h</code></td><td>LED pin defines (R=GPIO 4, G=16, B=17, active-LOW) + convenience macros (<code>CYD_LED_RED/GREEN/BLUE_ON/OFF()</code>, <code>CYD_LED_RGB_ON/OFF()</code>) + <code>initLEDS()</code>; city/country tables; nameday logic; screen/data types; moon phase; string helpers; HTTP constants</td></tr>
<tr><td>2</td><td><code>net/timezone.h/.cpp</code>, <code>net/ota.h/.cpp</code>, <code>net/weather_api.h/.cpp</code></td><td>IANA→POSIX mapping, timeapi.io lookup, OTA version check + flash, geocoding + Open-Meteo fetch</td></tr>
<tr><td>3</td><td><code>ui/theme.h/.cpp</code>, <code>ui/icons.h/.cpp</code>, <code>ui/clock_face.h/.cpp</code>, <code>ui/screens.h/.cpp</code></td><td>Theme colour helpers, all vector drawing, clock/date/weather rendering, all draw*Screen functions + keyboard</td></tr>
<tr><td>4</td><td><code>ui/touch_handler.h/.cpp</code></td><td>Entire 1,179-line <code>switch(currentState)</code> dispatch extracted from <code>loop()</code></td></tr>
<tr><td>5</td><td><code>util/credentials.h/.cpp</code>, <code>data/recent.h/.cpp</code>, <code>net/location.h/.cpp</code></td><td>Password obfuscation, recent cities NVS, country/city REST + embedded + Nominatim lookups</td></tr>
<tr><td>6</td><td><code>app/location.h/.cpp</code>, <code>hal/backlight.h/.cpp</code></td><td>syncRegion/applyLocation/loadSavedLocation, applyAutoDim</td></tr>
<tr><td>7</td><td><code>net/holidays.h/.cpp</code></td><td>Public holiday fetch (Nager.Date API); <code>handleHolidayUpdate()</code> day-change guard; <code>lookupISOCode</code> global (set by <code>lookupCountryEmbedded/RESTApi()</code>, persisted as NVS <code>isoCode</code>). <code>countryToISO()</code> in <code>string_utils</code> deleted — superseded by <code>lookupISOCode</code>.</td></tr>
</tbody>
</table>

**Dependency direction**: `main` → `app/ui` → `net` → `util/data` (no upward dependencies).



<!-- //  --- EOF --- // -->
