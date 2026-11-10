/*
 * CYD_DD HARDWARE DIAGNOSTIC FIRMWARE (standalone, not the clock app)
 *
 * Purpose: determine why (a) the WiFi scan list is empty and (b) touch
 * freezes after a while.  It continuously dumps raw hardware state to
 * serial and mirrors a short status line on the TFT.
 *
 * Flash + monitor:
 *     pio run -e diag -t upload -t monitor
 *
 * What to do while it runs:
 *   1. Touch the screen: z should jump above ~200, x/y should move.
 *   2. Press and HOLD the screen for ~10 s: watch whether z stays high.
 *   3. Leave it alone: every 12 s one WiFi scan variant runs
 *      (ACTIVE CN / ACTIVE 01 / PASSIVE) and prints what it sees.
 *   4. Serial commands:  r = reinit touch,  s = scan now
 *
 * Interpretation:
 *   - Touch: z always 0 while pressing  -> controller/SPI wiring problem
 *             z stuck >1000 forever     -> controller conversion wedged
 *             x/y/z healthy but app dies-> IRQ-line / app-logic issue
 *   - WiFi:  ACTIVE 01 sees nothing but PASSIVE sees networks -> channel/
 *             country issue (fixable in software)
 *             NOTHING seen in any mode -> radio/antenna hardware problem
 */

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ---- Same touch pins as the production firmware ----
#define T_CS   33
#define T_CLK  25
#define T_DIN  32
#define T_DOUT 39
#define T_IRQ  36

static TFT_eSPI tft = TFT_eSPI();
static XPT2046_Touchscreen ts( T_CS );

static const int Z_THRESHOLD = 200;

static unsigned long lastTouchPoll   = 0;
static unsigned long lastScan        = 0;
static int           scanRound       = 0;
static int           sameZCount      = 0;
static int           lastZ           = -1;

// ---------------------------------------------------------------- touch
static void touchReinit( const char *why ) {
    Serial.printf( "[T] --- reinit touch (%s) ---\n", why );
    ts.begin();
    ts.setRotation( 1 );
}

static void pollTouch() {
    TS_Point p = ts.getPoint();
    int irq = digitalRead( T_IRQ );

    // Serial: full detail every 300 ms
    Serial.printf( "[T] x=%4d y=%4d z=%4d irq=%d touched=%d\n",
                   p.x, p.y, p.z, irq, ts.touched() ? 1 : 0 );

    // Wedge detection: same non-zero z for many consecutive reads
    if ( p.z > Z_THRESHOLD && p.z == lastZ ) {
        if ( ++sameZCount == 20 ) {
            Serial.println( "[T] !! z frozen at same value for ~6 s — controller may be wedged, reinitting" );
            touchReinit( "frozen z" );
            sameZCount = 0;
        }
    }
    else {
        sameZCount = 0;
    }
    lastZ = p.z;

    // TFT: short live status
    static uint16_t lastZ4 = 0xFFFF;
    if ( (uint16_t)p.z != lastZ4 ) {          // only redraw when z changed
        lastZ4 = (uint16_t)p.z;
        tft.setTextDatum( TL_DATUM );
        tft.setTextColor( p.z >= Z_THRESHOLD ? TFT_GREEN : TFT_WHITE, TFT_BLACK );
        tft.fillRect( 0, 190, 320, 50, TFT_BLACK );
        tft.drawString( "TOUCH  x/y/z  (irq)", 8, 196, 2 );
        char buf[ 48 ];
        snprintf( buf, sizeof( buf ), "%4d %4d %4d  (%d)", p.x, p.y, p.z, irq );
        tft.drawString( buf, 8, 216, 2 );
    }
}

// ---------------------------------------------------------------- wifi
static void doScan( const char *tag, bool passive ) {
    WiFi.mode( WIFI_STA );
    WiFi.setSleep( false );

    const char *cc = ( strcmp( tag, "ACTIVE 01" ) == 0 ) ? "01" : "CN";
    esp_err_t rc = esp_wifi_set_country_code( cc, true );
    char now[ 4 ] = { 0 };
    esp_wifi_get_country_code( now );
    Serial.printf( "\n[W] === %s ===  set %s -> err=%d, active country: %s\n",
                   tag, cc, (int)rc, now );

    int n = WiFi.scanNetworks( false, true, passive, passive ? 120 : 300 );
    if ( n == WIFI_SCAN_RUNNING ) { Serial.println( "[W] scan returned RUNNING(-1)" ); n = 0; }
    if ( n == WIFI_SCAN_FAILED )  { Serial.println( "[W] scan returned FAILED(-2) — radio busy or not started" ); n = 0; }

    Serial.printf( "[W] %s scan -> %d networks\n", tag, n );
    int shown = 0;
    for ( int i = 0; i < n && shown < 20; i++ ) {
        String s = WiFi.SSID( i );
        if ( s.length() == 0 ) continue;
        Serial.printf( "    ch=%2d rssi=%3d  %s\n", WiFi.channel( i ), WiFi.RSSI( i ), s.c_str() );
        shown++;
    }
    if ( shown == 0 ) {
        Serial.println( "[W] (no named networks visible in this mode)" );
    }
    WiFi.scanDelete();

    // Mirror result on TFT
    tft.setTextDatum( TL_DATUM );
    tft.fillRect( 0, 0, 320, 120, TFT_BLACK );
    tft.setTextColor( TFT_CYAN, TFT_BLACK );
    tft.drawString( "DIAG  -  watch serial", 8, 4, 2 );
    tft.setTextColor( n > 0 ? TFT_GREEN : TFT_RED, TFT_BLACK );
    char buf[ 40 ];
    snprintf( buf, sizeof( buf ), "%s: %d nets", tag, n );
    tft.drawString( buf, 8, 26, 2 );
    tft.setTextColor( TFT_WHITE, TFT_BLACK );
    tft.drawString( "PRESS & HOLD the screen", 8, 60, 2 );
    tft.drawString( "z should go above ~200", 8, 80, 2 );
}

// ---------------------------------------------------------------- setup
void setup() {
    pinMode( TFT_BL, OUTPUT );
    digitalWrite( TFT_BL, HIGH );      // backlight on for the diag screen

    Serial.begin( 115200 );
    delay( 400 );
    Serial.println( "\n=== CYD_DD DIAGNOSTIC ===" );
    Serial.printf( "chip=%s rev=%d cores=%d flash=%u KB heap=%u\n",
                   ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores(),
                   (unsigned)( ESP.getFlashChipSize() / 1024 ), (unsigned)ESP.getFreeHeap() );

    tft.init();
    tft.setRotation( 1 );
    tft.fillScreen( TFT_BLACK );

    SPI.begin( T_CLK, T_DOUT, T_DIN );
    ts.begin();
    ts.setRotation( 1 );

    WiFi.mode( WIFI_STA );
    Serial.printf( "[W] MAC: %s\n", WiFi.macAddress().c_str() );

    doScan( "ACTIVE CN", false );
    lastScan = millis();
}

// ---------------------------------------------------------------- loop
void loop() {
    if ( millis() - lastTouchPoll > 300 ) {
        lastTouchPoll = millis();
        pollTouch();
    }

    if ( millis() - lastScan > 12000 ) {
        lastScan = millis();
        switch ( scanRound++ % 3 ) {
            case 0: doScan( "ACTIVE CN", false ); break;
            case 1: doScan( "ACTIVE 01", false ); break;
            case 2: doScan( "PASSIVE CN", true ); break;
        }
    }

    // serial commands
    while ( Serial.available() ) {
        char c = Serial.read();
        if ( c == 'r' ) touchReinit( "serial cmd" );
        if ( c == 's' ) doScan( "ACTIVE CN", false );
    }

    delay( 20 );
}
