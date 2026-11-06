#include "clock_face.h"
#include "cjk_text.h"
#include "theme.h"
#include "icons.h"

#include <TFT_eSPI.h>
#include "DSEG7Bold15pt7b.h"
#include <time.h>

#include "../data/app_state.h"
#include "../data/nameday.h"
#include "../net/holidays.h"
#include "../util/moon.h"
#include "../util/constants.h"
#include "../net/weather_api.h"

// ---------------------------------------------------------------------------
// Externs – defined in main.cpp
// ---------------------------------------------------------------------------
extern TFT_eSPI tft;

// Clock layout
extern const int clockX;
extern const int clockY;
extern const int radius;
extern int       lastHour;
extern int       lastMin;
extern int       lastSec;
extern bool      forceClockRedraw;

// Display/format state
extern bool isDigitalClock;
extern bool is12hFormat;
extern bool showDigitalSeconds;
extern bool isWhiteTheme;
extern int  themeMode;

// City/region
extern String cityName;
extern String selectedCountry;

// Weather data
extern bool  initialWeatherFetched;
extern int   weatherCode;
extern float currentTemp;
extern int   currentHumidity;
extern float currentWindSpeed;
extern int   currentWindDirection;
extern int   currentPressure;
extern bool  weatherUnitF;
extern bool  weatherUnitMph;
extern bool  weatherUnitInHg;
extern String sunriseTime;
extern String sunsetTime;
extern ForecastData forecast[ 2 ];
extern String forecastDay1Name;
extern String forecastDay2Name;
extern int    moonPhaseVal;

// Bitmap icons (defined in main.cpp)
extern const unsigned char icon_sunrise[];
extern const unsigned char icon_sunset[];

// DEGTORAD
static constexpr float DEGTORAD_CF = ( float )( PI / 180.0 );

// ---------------------------------------------------------------------------
// THEME_TECH (dark tech) / THEME_CARD (light cards) — digital-only faces
// ---------------------------------------------------------------------------
static bool isTechTheme() {
    return themeMode == THEME_TECH;
}

static bool isCardTheme() {
    return themeMode == THEME_CARD;
}

static bool isDigitalOnlyTheme() {
    return isTechTheme() || isCardTheme();
}

// Palette (values go through panelColor() — this panel swaps R/B)
static uint16_t techCyan()   { return panelColor( 0x2BD9E8 ); }
static uint16_t techCard()   { return panelColor( 0x1B1E22 ); }
static uint16_t techBorder() { return panelColor( 0x3A4046 ); }
static uint16_t techGray()   { return panelColor( 0x8A939B ); }
static uint16_t techWhite()  { return panelColor( 0xF2F5F7 ); }

static uint16_t cardNavy()   { return panelColor( 0x1F3A66 ); }
static uint16_t cardWhite()  { return panelColor( 0xFFFFFF ); }
static uint16_t cardEdge()   { return panelColor( 0xDDD6C2 ); }
static uint16_t cardTop()    { return panelColor( 0xC7DDF2 ); }
static uint16_t cardBottom() { return panelColor( 0xF3F7FB ); }

// Vertical gradient with rounded top/bottom corners (row-interpolated).
// Works on pre-swapped colours: interpolating the RGB565 fields of two
// pre-swapped endpoints stays pre-swapped.
static void gradientRoundRect( int x, int y, int w, int h, int r, uint16_t cTop, uint16_t cBot ) {
    int tr = ( cTop >> 11 ) & 0x1F, tg = ( cTop >> 5 ) & 0x3F, tb = cTop & 0x1F;
    int br = ( cBot >> 11 ) & 0x1F, bg_ = ( cBot >> 5 ) & 0x3F, bb = cBot & 0x1F;
    for ( int yy = 0; yy < h; yy++ ) {
        int inset = 0;
        if ( yy < r ) {
            int dy = r - yy;
            inset = r - ( int )sqrtf( ( float )( r * r - dy * dy ) );
        }
        else if ( yy >= h - r ) {
            int dy = r - ( h - 1 - yy );
            inset = r - ( int )sqrtf( ( float )( r * r - dy * dy ) );
        }
        float t = ( h > 1 ) ? ( float )yy / ( h - 1 ) : 0.0f;
        uint16_t c = ( uint16_t )( ( ( int )( tr + ( br - tr ) * t ) << 11 ) |
                                   ( ( int )( tg + ( bg_ - tg ) * t ) << 5 ) |
                                   ( int )( tb + ( bb - tb ) * t ) );
        tft.drawFastHLine( x + inset, y + yy, w - 2 * inset, c );
    }
}

// Big HH:MM — uses the 75px font 8 when it fits the given width, else 48px font 6
static void drawBigTime( const char *str, int cx, int cy, uint16_t color, uint16_t bg, int maxW ) {
    tft.setTextDatum( MC_DATUM );
    tft.setTextColor( color, bg );
    if ( tft.textWidth( str, 8 ) <= maxW ) {
        tft.setTextFont( 8 );
    }
    else {
        tft.setTextFont( 6 );
    }
    tft.drawString( str, cx, cy );
    tft.setTextFont( 0 );
    tft.setTextDatum( TL_DATUM );
}

// ---------------------------------------------------------------------------
// Clock-face sprite  — 140×140 px, rendered off-screen then pushed atomically
// so no intermediate state (tick erase → ticks redraw → hand redraw) is visible.
// ---------------------------------------------------------------------------
static TFT_eSprite clockSprite( &tft );
static bool        spriteCreated = false;
static int         spriteX = 0, spriteY = 0;  // top-left of sprite on screen
static int         sCX = 0,     sCY = 0;       // clock centre in sprite coords

// Dial geometry: the WOOD theme uses a big antique dial per mockup B —
// radius 90 centred at (205,114): clears the right frame (inner edge ~308)
// and leaves ~16 px of whitespace between the left column and the bezel.
static int dialRadius() {
    return ( themeMode == THEME_WOOD ) ? 90 : radius;
}

static int dialCx() {
    return ( themeMode == THEME_WOOD ) ? 205 : clockX;
}

static int dialCy() {
    return ( themeMode == THEME_WOOD ) ? 114 : clockY;
}

static void createClockSprite() {
    static int spriteTheme = -1;           // theme the sprite was built for
    if ( spriteCreated && spriteTheme == themeMode ) {
        return;
    }
    if ( spriteCreated ) {
        clockSprite.deleteSprite();        // theme changed → rebuild at new size
        spriteCreated = false;
    }
    int R   = dialRadius();
    int sz  = ( R + 3 ) * 2;
    spriteX = dialCx() - ( R + 3 );
    spriteY = dialCy() - ( R + 3 );
    sCX     = R + 3;
    sCY     = R + 3;
    clockSprite.setColorDepth( 16 );
    clockSprite.createSprite( sz, sz );
    spriteTheme = themeMode;
    spriteCreated = true;
}

void drawClockStatic() {
    if ( isDigitalClock ) {
        return;    // Nothing to draw in digital mode
    }

    // Draw minute and hour tick marks
    for ( int i = 0; i < 60; i++ ) {
        float ang = ( i * 6 - 90 ) * DEGTORAD_CF;
        int r1 = ( i % 5 == 0 ) ? ( radius - 10 ) : ( radius - 5 );
        uint16_t color;
        if ( i % 5 == 0 ) {
            color = getTextColor();
        }
        else {
            color = ( themeMode == THEME_YELLOW ) ? 0x0010 : TFT_DARKGREY;
        }
        tft.drawLine( clockX + cos( ang ) * radius, clockY + sin( ang ) * radius, clockX + cos( ang ) * r1, clockY + sin( ang ) * r1, color );
    }

    // Draw numbers 1-12
    tft.setTextColor( getTextColor() );
    tft.setTextDatum( MC_DATUM );
    tft.setFreeFont( &FreeSans9pt7b );

    for ( int h = 1; h <= 12; h++ ) {
        float angle = ( h * 30 - 90 ) * DEGTORAD_CF;
        int x = clockX + cos( angle ) * ( radius - 22 );
        int y = clockY + sin( angle ) * ( radius - 22 );
        cjkDraw( String( h ), x, y );
    }
}

// Breguet-style clock hand: shaft from a tail counterweight, a solid "moon"
// disc near the tip, and a pointed tip beyond it — the classic ornate look
// from the WOOD mockup, built from primitives the sprite can draw fast.
static void drawBreguetHand( TFT_eSprite &spr, int cx, int cy, float aDeg,
                             int len, int tail, int shaftW, int moonR,
                             uint16_t color )
{
    float a  = aDeg * DEGTORAD_CF;
    float ux = cos( a ), uy = sin( a );
    int   mx = cx + ( int )( ux * ( len * 0.62f ) );
    int   my = cy + ( int )( uy * ( len * 0.62f ) );

    // tail (counterweight side) + shaft
    spr.drawWideLine( cx - ( int )( ux * tail ), cy - ( int )( uy * tail ),
                      mx, my, shaftW, color, color );
    // moon disc
    spr.fillCircle( mx, my, moonR, color );
    // pointed tip flaring out of the moon
    spr.fillTriangle(
        mx + ( int )( -uy * moonR ), my + ( int )( ux * moonR ),
        mx - ( int )( -uy * moonR ), my - ( int )( ux * moonR ),
        cx + ( int )( ux * len ), cy + ( int )( uy * len ), color );
}

// Darken an RGB565 colour towards black (used to fake soft shadows —
// RGB565 has no alpha, so shadows are drawn as a darker shade of the dial).
static inline uint16_t darken565( uint16_t c, uint8_t shift )
{
    uint16_t r = ( c >> 11 ) & 0x1F;
    uint16_t g = ( c >> 5 ) & 0x3F;
    uint16_t b = c & 0x1F;
    r -= r >> shift;
    g -= g >> shift;
    b -= b >> shift;
    return ( uint16_t )( ( r << 11 ) | ( g << 5 ) | b );
}

void drawClockFace() {
    tft.fillScreen( getBgColor() );
    if ( isTechTheme() ) {
        // Short cyan tick in the top-right corner (mockup detail)
        tft.drawFastHLine( 282, 12, 28, techCyan() );
        forceClockRedraw = true;
        return;
    }
    if ( isCardTheme() ) {
        // Right-hand gradient time card (mockup detail)
        gradientRoundRect( 142, 10, 172, 224, 18, cardTop(), cardBottom() );
        tft.drawRoundRect( 142, 10, 172, 224, 18, cardEdge() );
        forceClockRedraw = true;
        return;
    }
    if ( themeMode == THEME_WOOD ) {
        drawWoodTexture( 0, 0, tft.width(), tft.height() );
        drawWoodFrame();
    }
    if ( !isDigitalClock ) {
        createClockSprite();   // allocate sprite (no-op if already done)
        if ( themeMode == THEME_WOOD ) {
            // Vintage face: the sprite (redrawn every second) paints the whole
            // bezel + dial with a transparent surround, so nothing is needed
            // here — the wood grain stays visible around the clock.
        }
        else {
            tft.drawCircle( clockX, clockY, radius + 2, getTextColor() );
            drawClockStatic();
        }
    }
    forceClockRedraw = true;
}

void drawDateAndWeek( const struct tm *ti ) {
    // ── THEME_TECH: date under the seconds — "2025-05-28 周三" ────────────────
    if ( isTechTheme() ) {
        uint16_t bg = getBgColor();
        tft.fillRect( 108, 172, 208, 30, bg );
        char dateBuf[ 16 ];
        strftime( dateBuf, sizeof( dateBuf ), "%Y-%m-%d", ti );
        const char *dayCN[] = { "日", "一", "二", "三", "四", "五", "六" };
        String s = String( dateBuf ) + " 周" + dayCN[ ti->tm_wday ];
        tft.setTextDatum( MC_DATUM );
        tft.setTextColor( techWhite(), bg );
        cjkDraw( s, 212, 187, 2 );
        tft.setTextDatum( TL_DATUM );
        return;
    }

    // ── THEME_CARD: date inside the gradient card — "2024年6月2日 星期日" ─────
    if ( isCardTheme() ) {
        // Repaint the gradient band behind the date, then the text
        gradientRoundRect( 150, 150, 156, 32, 0, cardTop(), cardBottom() );
        const char *dayCN[] = { "日", "一", "二", "三", "四", "五", "六" };
        String s = String( ti->tm_year + 1900 ) + "年" + String( ti->tm_mon + 1 ) + "月" +
                   String( ti->tm_mday ) + "日 星期" + dayCN[ ti->tm_wday ];
        tft.setTextDatum( MC_DATUM );
        tft.setTextColor( cardNavy(), cardBottom() );
        cjkDraw( s, 228, 166, 2 );
        tft.setTextDatum( TL_DATUM );
        return;
    }

    // THEME_WOOD: the mockup keeps the date in the left gold panel and the
    // whole right side belongs to the dial sprite — nothing to draw here.
    // (The old repaint rect at 155,160 overlapped the dial's lower half.)
    if ( themeMode == THEME_WOOD ) {
        return;
    }

    uint16_t dateColor = getTextColor();
    if ( themeMode == THEME_YELLOW ) {
        dateColor = TFT_BLACK;
    }

    tft.setFreeFont( NULL );
    tft.setTextColor( dateColor, getBgColor() );
    tft.setTextDatum( MC_DATUM );

    tft.fillRect( 155, 160, 165, 80, getBgColor() );
    if ( themeMode == THEME_WOOD ) {
        drawWoodTexture( 155, 160, 165, 80 );   // restore grain under the text block
    }

    char dateBuf[ 30 ];
    strftime( dateBuf, sizeof( dateBuf ), "%Y年%m月%d日", ti );
    cjkDraw( String( dateBuf ), clockX, 175, 2 );

    int weekNum = 0;
    char weekBuf[ 20 ];
    strftime( weekBuf, sizeof( weekBuf ), "%V", ti );
    weekNum = atoi( weekBuf );

    const char *dayNames[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
    String dayStr = String( dayNames[ ti->tm_wday ] );
    String weekStr = "第" + String( weekNum ) + "周 " + dayStr;
    cjkDraw( weekStr, clockX, 193, 2 );

    if ( cityName != "" ) {
        if ( themeMode == THEME_YELLOW ) {
            tft.setTextColor( 0x0220, getBgColor() ); // Dark green
        }
        else if ( themeMode == THEME_WOOD ) {
            tft.setTextColor( getBrassColor(), getBgColor() );  // brass on walnut
        }
        else {
            tft.setTextColor( TFT_SKYBLUE, getBgColor() );
        }
        cjkDraw( cityName, clockX, 211, 2 );
    }

    // Holiday line takes priority; nameday shown only for Czech Republic when no holiday
    if ( holidayValid && todayHoliday.length() > 0 ) {
        uint16_t holidayColor;
        if ( themeMode == THEME_YELLOW ) {
            holidayColor = 0x0220;                                  // dark green on yellow bg
        }
        else {
            holidayColor = isWhiteTheme ? TFT_DARKGREEN : TFT_RED;  // red on dark themes
        }
        tft.setTextColor( holidayColor, getBgColor() );
        cjkDraw( todayHoliday, clockX, 227, 1 );
    }
    else if ( namedayValid && todayNameday != "--" && selectedCountry == "Czech Republic" ) {
        uint16_t namedayColor;
        if ( themeMode == THEME_YELLOW ) {
            namedayColor = 0x0220;
        }
        else {
            namedayColor = isWhiteTheme ? TFT_DARKGREEN : TFT_ORANGE;
        }
        tft.setTextColor( namedayColor, getBgColor() );
        cjkDraw( "命名日: " + todayNameday, clockX, 227, 1 );
    }
    else {
        // Clear the line if neither applies (e.g. country changed)
        tft.fillRect( 155, 220, 165, 17, getBgColor() );
    }
}

void drawDigitalClock( int h, int m, int s ) {
    uint16_t clockColor = getTextColor();
    uint16_t bgColor = getBgColor();

    if ( themeMode == THEME_BLUE ) {
        clockColor = TFT_WHITE;
    }
    if ( themeMode == THEME_YELLOW ) {
        clockColor = TFT_BLACK;
    }

    int displayH = h;
    bool isPM = false;

    if ( is12hFormat ) {
        if ( displayH >= 12 ) {
            isPM = true;
            if ( displayH > 12 ) {
                displayH -= 12;
            }
        }
        else {
            if ( displayH == 0 ) {
                displayH = 12;
            }
        }
    }

    char timeStr[ 6 ];
    if ( is12hFormat ) {
        sprintf( timeStr, "%d:%02d", displayH, m );   // no leading zero in 12h
    }
    else {
        sprintf( timeStr, "%02d:%02d", displayH, m ); // leading zero in 24h
    }

    // Compute font 7 metrics once — used for layout of all elements below.
    int fh7      = tft.fontHeight( 7 );
    int maxTimeW = tft.textWidth( "12:59", 7 );

    tft.setTextDatum( MC_DATUM );
    tft.setTextColor( clockColor, bgColor );

    // Redraw HH:MM only when the string changes (or a full redraw is forced).
    static char prevTimeStr[ 6 ] = "";
    if ( forceClockRedraw || strcmp( timeStr, prevTimeStr ) != 0 ) {
        tft.fillRect( clockX - maxTimeW / 2, clockY - fh7 / 2, maxTimeW, fh7, bgColor );
        cjkDraw( timeStr, clockX, clockY, 7 );
        strncpy( prevTimeStr, timeStr, sizeof( prevTimeStr ) );
    }

    // Seconds in DSEG7 Bold 15pt (GFX free font) — only when showDigitalSeconds is true.
    // TFT_eSPI renders GFX font glyphs pixel-by-pixel, so setTextColor(fg, bg) fills
    // the background per-glyph — no separate fillRect needed, no blank-frame flicker.
    tft.setFreeFont( &DSEG7Bold15pt7b );
    int secFontH = tft.fontHeight();
    // Centre seconds exactly between the bottom of HH:MM and the top of the date line
    // (date drawn at y=175, font 2, 16px → top of date = 175 - fontHeight(2)/2).
    int bottomHHMM = clockY + fh7 / 2;
    int topDate    = 175 - tft.fontHeight( 2 ) / 2;
    int secY       = ( bottomHHMM + topDate ) / 2;
    if ( showDigitalSeconds ) {
        char secStr[ 3 ];
        sprintf( secStr, "%02d", s );
        tft.setTextColor( getSecHandColor(), bgColor );
        cjkDraw( secStr, clockX, secY );
    }
    else if ( forceClockRedraw ) {
        // Clear the seconds area when hidden (only needed on forced redraws —
        // per-glyph bg fill keeps it clean during normal second ticks).
        int secW = tft.textWidth( "00" );
        tft.fillRect( clockX - secW / 2, secY - secFontH / 2, secW, secFontH, bgColor );
    }
    tft.setTextFont( 0 );  // clear free font, restore default

    // AM/PM indicator — only in 12h mode.
    // Fixed X: 4 px to the right of the widest time string ("12:59"), so the dot never
    // shifts when the display is only 3 digits wide.
    // AM circle: top of circle = top of main digits (clockY - fh7/2).
    // PM circle: bottom of circle = bottom of main digits (clockY + fh7/2).
    // Circles are only redrawn when isPM changes (or a full redraw is forced) to avoid
    // the every-second erase/redraw touching the HH:MM glyph area and causing flicker.
    const int circR = 5;
    // Clamp so the circle never clips the right screen edge when the font is wide.
    const int circX = min( clockX + maxTimeW / 2 + circR + 4, ( int )tft.width() - circR - 1 );
    const int amY   = clockY - fh7 / 2 + circR;
    const int pmY   = clockY + fh7 / 2 - circR;
    static bool prevIsPM = !isPM;  // initialise to opposite so first call always draws
    if ( forceClockRedraw || isPM != prevIsPM ) {
        tft.fillCircle( circX, amY, circR, bgColor );
        tft.fillCircle( circX, pmY, circR, bgColor );
        if ( is12hFormat ) {
            tft.fillCircle( circX, isPM ? pmY : amY, circR, TFT_ORANGE );
        }
        prevIsPM = isPM;
    }

    forceClockRedraw = false;   // consumed — reset so guards fire only on real changes
}

void updateHands( int h, int m, int s ) {
    if ( isDigitalClock ) {
        drawDigitalClock( h, m, s );
        return;
    }

    // ── THEME_TECH: huge white HH:MM + cyan seconds on the right ──────────────
    if ( isTechTheme() ) {
        uint16_t bg = getBgColor();
        char timeStr[ 6 ];
        sprintf( timeStr, "%02d:%02d", h, m );
        static char prevTime[ 6 ] = "";
        if ( forceClockRedraw || strcmp( timeStr, prevTime ) != 0 ) {
            tft.fillRect( 108, 44, 208, 96, bg );
            drawBigTime( timeStr, 212, 92, techWhite(), bg, 204 );
            strncpy( prevTime, timeStr, sizeof( prevTime ) );
        }

        char secStr[ 3 ];
        sprintf( secStr, "%02d", s );
        tft.setTextDatum( MC_DATUM );
        tft.setFreeFont( &FreeSansBold18pt7b );
        tft.setTextColor( techCyan(), bg );
        tft.fillRect( 178, 140, 68, 34, bg );
        tft.drawString( secStr, 212, 158 );
        tft.setFreeFont( NULL );
        tft.setTextDatum( TL_DATUM );
        forceClockRedraw = false;
        return;
    }

    // ── THEME_CARD: navy HH:MM inside the gradient card (no seconds) ──────────
    if ( isCardTheme() ) {
        char timeStr[ 6 ];
        sprintf( timeStr, "%02d:%02d", h, m );
        static char prevTimeC[ 6 ] = "";
        if ( forceClockRedraw || strcmp( timeStr, prevTimeC ) != 0 ) {
            gradientRoundRect( 150, 52, 156, 92, 0, cardTop(), cardBottom() );
            drawBigTime( timeStr, 228, 98, cardNavy(), cardBottom(), 160 );
            strncpy( prevTimeC, timeStr, sizeof( prevTimeC ) );
        }
        forceClockRedraw = false;
        return;
    }

    createClockSprite();   // no-op after first call; safe even if drawClockFace() skipped

    uint16_t bgColor       = getBgColor();
    uint16_t mainHandColor = getTextColor();
    uint16_t secColor      = getSecHandColor();

    // ── THEME_WOOD: vintage brass/cream dial with Roman numerals ──────────────
    if ( themeMode == THEME_WOOD ) {
        // Opaque sprite — the transparent-key push path renders colours
        // unpredictably on this panel, so push like the other themes do.
        // Fill the background with the SAME wood grain as the screen (pure
        // function of the on-screen y), so the square sprite blends seamlessly
        // into the textured panel — no visible flat rectangle behind the dial.
        const int szW = dialRadius() + 3;   // half of the sprite size
        for ( int yy = 0; yy < szW * 2; yy++ ) {
            clockSprite.drawFastHLine( 0, yy, szW * 2, woodRowColor( spriteY + yy ) );
        }

        const int  R        = dialRadius();
        const uint16_t brass     = getBrassColor();
        const uint16_t brassDark = getBrassDarkColor();
        const uint16_t brassHi   = panelColor( 0xE8C87A );  // highlight brass
        const uint16_t dial      = getDialColor();
        const uint16_t dialShade = panelColor( 0xE3D6B8 );  // slightly aged cream ring
        const uint16_t ink       = getInkColor();

        // Bezel: thick brass ring — dark outer rim, highlight, inner fillet
        clockSprite.fillCircle( sCX, sCY, R + 2, brassDark );
        clockSprite.fillCircle( sCX, sCY, R, brass );
        clockSprite.drawCircle( sCX, sCY, R - 1, brassHi );
        clockSprite.fillCircle( sCX, sCY, R - 10, dial );
        clockSprite.drawCircle( sCX, sCY, R - 11, brassDark );
        clockSprite.drawCircle( sCX, sCY, R - 18, dialShade );   // aged outer band
        clockSprite.drawCircle( sCX, sCY, R - 19, dial );

        // Decorative rivets on the bezel (4 diagonals)
        for ( int i = 0; i < 4; i++ ) {
            float ang = ( 45 + i * 90 ) * DEGTORAD_CF;
            int   sx  = sCX + ( int )( cos( ang ) * ( R - 5 ) );
            int   sy  = sCY + ( int )( sin( ang ) * ( R - 5 ) );
            clockSprite.fillCircle( sx, sy, 3, brassHi );
            clockSprite.drawCircle( sx, sy, 3, brassDark );
            clockSprite.drawLine( sx - 1, sy, sx + 1, sy, brassDark );  // slot
        }

        // Thin ring around the tick band (per mockup)
        clockSprite.drawCircle( sCX, sCY, R - 20, ink );

        // Tick marks on the cream dial
        for ( int i = 0; i < 60; i++ ) {
            float ang = ( i * 6 - 90 ) * DEGTORAD_CF;
            if ( i % 5 == 0 ) {
                clockSprite.drawWideLine(
                    sCX + ( int )( cos( ang ) * ( R - 22 ) ), sCY + ( int )( sin( ang ) * ( R - 22 ) ),
                    sCX + ( int )( cos( ang ) * ( R - 32 ) ), sCY + ( int )( sin( ang ) * ( R - 32 ) ),
                    2, ink, ink );
            }
            else {
                clockSprite.drawLine(
                    sCX + ( int )( cos( ang ) * ( R - 22 ) ), sCY + ( int )( sin( ang ) * ( R - 22 ) ),
                    sCX + ( int )( cos( ang ) * ( R - 27 ) ), sCY + ( int )( sin( ang ) * ( R - 27 ) ),
                    ink );
            }
        }

        // Roman numerals (classic IIII at 4 o'clock), refined serif
        clockSprite.setFreeFont( &FreeSerif9pt7b );
        clockSprite.setTextDatum( MC_DATUM );
        clockSprite.setTextColor( ink, dial );
        const char *romans[] = {"XII", "I", "II", "III", "IIII", "V", "VI", "VII", "VIII", "IX", "X", "XI"};
        for ( int n = 0; n < 12; n++ ) {
            int   h   = ( n == 0 ) ? 12 : n;
            float ang = ( h * 30 - 90 ) * DEGTORAD_CF;
            clockSprite.drawString( romans[ n ],
                                    sCX + ( int )( cos( ang ) * ( R - 42 ) ),
                                    sCY + ( int )( sin( ang ) * ( R - 42 ) ) );
        }
        clockSprite.setFreeFont( NULL );

        // Hands: Breguet-style with soft drop shadows, like the brass mockup.
        // Second hand kept short of the numerals ring so the dial stays balanced.
        float hA = ( ( h % 12 ) + ( m / 60.0f ) ) * 30.0f - 90.0f;
        float mA = m * 6.0f - 90.0f;
        float sA = s * 6.0f - 90.0f;
        const int    shX    = 1;                  // subtle 1px shadow: light top-left
        const int    shY    = 1;
        const uint16_t shadow = darken565( dial, 2 );

        // --- shadow pass (offset down-right) ---
        drawBreguetHand( clockSprite, sCX + shX, sCY + shY, hA, R - 52, 15, 4, 5, shadow );
        drawBreguetHand( clockSprite, sCX + shX, sCY + shY, mA, R - 30, 15, 3, 4, shadow );
        clockSprite.drawWideLine(
            sCX + shX + ( int )( cos( sA * DEGTORAD_CF ) * 10 ),
            sCY + shY + ( int )( sin( sA * DEGTORAD_CF ) * 10 ),
            sCX + shX + ( int )( cos( sA * DEGTORAD_CF ) * ( R - 48 ) ),
            sCY + shY + ( int )( sin( sA * DEGTORAD_CF ) * ( R - 48 ) ),
            1, shadow, shadow );
        clockSprite.fillCircle( sCX + shX, sCY + shY, 5, shadow );

        // --- real hands ---
        // Length order: minute (reaches ticks) > second (stops inside the
        // numerals ring at R-42) > hour. Second hand never crosses the numerals.
        drawBreguetHand( clockSprite, sCX, sCY, hA, R - 52, 15, 4, 5, ink );
        drawBreguetHand( clockSprite, sCX, sCY, mA, R - 30, 15, 3, 4, ink );
        // second: hairline, short of the numerals, small counterweight
        clockSprite.drawWideLine(
            sCX + ( int )( cos( sA * DEGTORAD_CF ) * 10 ),
            sCY + ( int )( sin( sA * DEGTORAD_CF ) * 10 ),
            sCX + ( int )( cos( sA * DEGTORAD_CF ) * ( R - 48 ) ),
            sCY + ( int )( sin( sA * DEGTORAD_CF ) * ( R - 48 ) ),
            1, ink, ink );
        clockSprite.fillCircle(
            sCX - ( int )( cos( sA * DEGTORAD_CF ) * 10 ),
            sCY - ( int )( sin( sA * DEGTORAD_CF ) * 10 ), 2, ink );
        // hub with its own shadow
        clockSprite.fillCircle( sCX + shX, sCY + shY, 5, shadow );
        clockSprite.fillCircle( sCX, sCY, 5, brass );
        clockSprite.drawCircle( sCX, sCY, 5, brassDark );
        clockSprite.fillCircle( sCX, sCY, 2, ink );

        clockSprite.pushSprite( spriteX, spriteY );
        return;
    }

    // ── Render full clock face into sprite then push in one write (no flicker) ──
    clockSprite.fillSprite( bgColor );

    // Outer border circle
    clockSprite.drawCircle( sCX, sCY, radius + 2, mainHandColor );

    // Tick marks
    for ( int i = 0; i < 60; i++ ) {
        float    ang   = ( i * 6 - 90 ) * DEGTORAD_CF;
        int      r1    = ( i % 5 == 0 ) ? ( radius - 10 ) : ( radius - 5 );
        uint16_t tkcol = ( i % 5 == 0 )
                         ? mainHandColor
                         : ( ( themeMode == THEME_YELLOW ) ? ( uint16_t )0x0010 : ( uint16_t )TFT_DARKGREY );
        clockSprite.drawLine(
            sCX + ( int )( cos( ang ) * radius ), sCY + ( int )( sin( ang ) * radius ),
            sCX + ( int )( cos( ang ) * r1     ), sCY + ( int )( sin( ang ) * r1     ),
            tkcol );
    }

    // Hour numerals
    clockSprite.setFreeFont( &FreeSans9pt7b );
    clockSprite.setTextDatum( MC_DATUM );
    clockSprite.setTextColor( mainHandColor );
    for ( int n = 1; n <= 12; n++ ) {
        float ang = ( n * 30 - 90 ) * DEGTORAD_CF;
        clockSprite.drawString( String( n ),
                                sCX + ( int )( cos( ang ) * ( radius - 22 ) ),
                                sCY + ( int )( sin( ang ) * ( radius - 22 ) ) );
    }
    clockSprite.setFreeFont( NULL );

    // Hands — Breguet style with drop shadows, consistent with the WOOD dial
    float hA = ( ( h % 12 ) + ( m / 60.0f ) ) * 30.0f - 90.0f;
    float mA = m * 6.0f  - 90.0f;
    float sA = s * 6.0f  - 90.0f;
    const uint16_t handShadow = darken565( (uint16_t)bgColor, 1 );

    // shadow pass
    drawBreguetHand( clockSprite, sCX + 2, sCY + 2, hA, radius - 35, 12, 3, 4, handShadow );
    drawBreguetHand( clockSprite, sCX + 2, sCY + 2, mA, radius - 20, 12, 2, 3, handShadow );
    clockSprite.drawWideLine(
        sCX + 2 + ( int )( cos( sA * DEGTORAD_CF ) * 10 ),
        sCY + 2 + ( int )( sin( sA * DEGTORAD_CF ) * 10 ),
        sCX + 2 + ( int )( cos( sA * DEGTORAD_CF ) * ( radius - 22 ) ),
        sCY + 2 + ( int )( sin( sA * DEGTORAD_CF ) * ( radius - 22 ) ),
        1, handShadow, handShadow );
    clockSprite.fillCircle( sCX + 2, sCY + 2, 3, handShadow );

    // real hands — second hand short of the numerals
    drawBreguetHand( clockSprite, sCX, sCY, hA, radius - 35, 12, 3, 4, mainHandColor );
    drawBreguetHand( clockSprite, sCX, sCY, mA, radius - 20, 12, 2, 3, mainHandColor );
    clockSprite.drawWideLine(
        sCX + ( int )( cos( sA * DEGTORAD_CF ) * 10 ),
        sCY + ( int )( sin( sA * DEGTORAD_CF ) * 10 ),
        sCX + ( int )( cos( sA * DEGTORAD_CF ) * ( radius - 22 ) ),
        sCY + ( int )( sin( sA * DEGTORAD_CF ) * ( radius - 22 ) ),
        1, secColor, secColor );
    clockSprite.fillCircle(
        sCX - ( int )( cos( sA * DEGTORAD_CF ) * 10 ),
        sCY - ( int )( sin( sA * DEGTORAD_CF ) * 10 ), 2, secColor );
    clockSprite.fillCircle( sCX, sCY, 3, TFT_LIGHTGREY );

    // Push sprite — single SPI burst, no intermediate state on screen
    clockSprite.pushSprite( spriteX, spriteY );
}

void drawWeatherSection() {
    uint16_t bg = getBgColor();
    uint16_t txt = getTextColor();
    uint16_t txtContrast = TFT_SKYBLUE;
    uint16_t lineColor = TFT_DARKGREY;

    if ( themeMode == THEME_BLUE ) {
        txtContrast = TFT_YELLOW;
    }
    else if ( themeMode == THEME_YELLOW ) {
        txtContrast = TFT_BLACK;
    }
    else if ( themeMode == THEME_WOOD ) {
        txtContrast = getBrassColor();     // brass accents on walnut
        lineColor   = getBrassDarkColor();
    }

    tft.fillRect( 0, 0, 155, 95, bg );
    tft.fillRect( 0, 105, 155, 100, bg );
    tft.fillRect( 0, 206, 155, 34, bg );
    if ( themeMode == THEME_WOOD ) {
        // Repaint the procedural grain over the cleared weather panel
        drawWoodTexture( 0, 0, 155, 95 );
        drawWoodTexture( 0, 105, 155, 100 );
        drawWoodTexture( 0, 206, 155, 34 );
    }

    if ( !initialWeatherFetched ) {
        tft.setTextColor( txt, bg );
        tft.setTextDatum( MC_DATUM );
        cjkDraw( "加载中...", 75, 120 );
        return;
    }

    // ── THEME_TECH: three dark info cards with cyan accents (mockup) ─────────
    if ( isTechTheme() ) {
        const uint16_t cardBg = techCard();
        const uint16_t border = techBorder();
        const uint16_t cyan   = techCyan();
        const uint16_t gray   = techGray();
        const uint16_t white  = techWhite();

        tft.fillRect( 0, 0, 104, 240, bg );

        auto card = [ & ]( int x, int y, int w, int h ) {
            tft.fillRoundRect( x, y, w, h, 8, cardBg );
            tft.drawRoundRect( x, y, w, h, 8, border );
        };

        // ── TEMP card ──
        card( 6, 8, 94, 62 );
        tft.setTextColor( gray, cardBg );
        tft.setTextDatum( TL_DATUM );
        cjkDraw( "TEMP", 14, 14, 1 );
        float dispTemp = weatherUnitF ? ( currentTemp * 9.0 / 5.0 + 32 ) : currentTemp;
        String t = String( ( int )dispTemp );
        tft.setFreeFont( &FreeSansBold24pt7b );
        tft.setTextColor( white, cardBg );
        int tw = tft.textWidth( t );
        int x0 = 53 - ( tw + 12 ) / 2;
        tft.drawString( t, x0, 26 );
        tft.setFreeFont( NULL );
        drawDegreeCircle( x0 + tw + 5, 40, 4, white );
        tft.drawFastHLine( 16, 64, 62, cyan );

        // ── WEATHER card ──
        card( 6, 78, 94, 78 );
        tft.setTextColor( gray, cardBg );
        cjkDraw( "WEATHER", 14, 84, 1 );
        drawWeatherIconVector( weatherCode, 18, 96 );
        tft.setTextDatum( MC_DATUM );
        tft.setTextColor( white, cardBg );
        cjkDraw( getWeatherDesc( weatherCode ), 53, 144, 2 );
        tft.setTextDatum( TL_DATUM );

        // ── ENV card ──
        card( 6, 164, 94, 68 );
        tft.drawFastHLine( 8, 166, 90, cyan );
        tft.setTextColor( gray, cardBg );
        cjkDraw( "ENV", 14, 170, 1 );
        tft.setTextDatum( MC_DATUM );
        cjkDraw( "湿度", 29, 192, 1 );
        tft.setTextColor( white, cardBg );
        cjkDraw( String( currentHumidity ) + "%", 29, 212, 2 );
        tft.setTextColor( gray, cardBg );
        cjkDraw( "风速", 76, 192, 1 );
        tft.setTextColor( white, cardBg );
        cjkDraw( String( currentWindSpeed, 1 ) + "m/s", 76, 212, 2 );
        tft.drawFastVLine( 52, 186, 40, border );
        tft.setTextDatum( TL_DATUM );
        return;
    }

    // ── THEME_CARD: two white cards — temp card + date card (mockup) ─────────
    if ( isCardTheme() ) {
        tft.fillRect( 0, 0, 138, 240, bg );

        // Card 1: sun icon + big navy temperature
        tft.fillRoundRect( 8, 10, 124, 68, 14, cardWhite() );
        tft.drawRoundRect( 8, 10, 124, 68, 14, cardEdge() );
        drawWeatherIconVector( weatherCode, 16, 26 );
        float dispTemp = weatherUnitF ? ( currentTemp * 9.0 / 5.0 + 32 ) : currentTemp;
        String t = String( ( int )dispTemp );
        tft.setFreeFont( &FreeSansBold24pt7b );
        tft.setTextColor( cardNavy(), cardWhite() );
        tft.setTextDatum( TL_DATUM );
        int tw = tft.textWidth( t );
        tft.drawString( t, 58, 24 );
        tft.setFreeFont( NULL );
        drawDegreeCircle( 58 + tw + 5, 38, 4, cardNavy() );

        // Card 2: date + weather icon
        tft.fillRoundRect( 8, 88, 124, 104, 14, cardWhite() );
        tft.drawRoundRect( 8, 88, 124, 104, 14, cardEdge() );
        struct tm ti;
        if ( getLocalTime( &ti ) ) {
            const char *dayCN[] = { "日", "一", "二", "三", "四", "五", "六" };
            String d = String( ti.tm_year + 1900 ) + "年" + String( ti.tm_mon + 1 ) + "月" +
                       String( ti.tm_mday ) + "日";
            tft.setTextDatum( TL_DATUM );
            tft.setTextColor( cardNavy(), cardWhite() );
            cjkDraw( d, 18, 104, 2 );
            cjkDraw( String( "星期" ) + dayCN[ ti.tm_wday ], 18, 128, 2 );
        }
        drawWeatherIconVectorSmall( weatherCode, 72, 146 );
        return;
    }

    // ── THEME_WOOD: gold-embossed panel per mockup B ─────────────────────────
    // Left column only: icon / big temp / RH / stacked date / city.
    // Forecast, wind, pressure, sunrise/sunset and moon are omitted (mockup).
    if ( themeMode == THEME_WOOD ) {
        // Repaint the full-height left column with fresh grain, then draw the panel
        drawWoodTexture( 0, 0, 155, 240 );

        const uint16_t gold  = panelColor( 0xD0A73F );   // saturated gold
        const uint16_t goldD = panelColor( 0x403010 );   // emboss shadow

        // Layout measured from the mockup (1024x768 → 320x240, scale 0.3125):
        // one vertical axis at x≈55; vertical rhythm icon 41 / temp 78 /
        // rule 108 / RH 130 / rule 152 / SAT 166 / day 186 / month 206.
        // Content stays clear of the carved frame (inner edge ≈ 9 px).
        const int ax = 55;

        // Weather icon (sun+cloud), centred on the axis
        drawWeatherIconVector( weatherCode, ax - 20, 23 );

        // Ornament: short symmetric brass rule with a diamond centre
        auto ornament = [ & ]( int y ) {
            tft.drawFastHLine( ax - 26, y, 16, getBrassDarkColor() );
            tft.drawFastHLine( ax + 10, y, 16, getBrassDarkColor() );
            tft.fillCircle( ax, y, 2, getBrassColor() );
            tft.drawCircle( ax, y, 2, getBrassDarkColor() );
        };

        // Big gold temperature — digits centred, small degree ring top-right
        float dispTemp = weatherUnitF ? ( currentTemp * 9.0 / 5.0 + 32 ) : currentTemp;
        String tempStr = String( ( int )dispTemp );
        tft.setTextDatum( MC_DATUM );
        tft.setFreeFont( &FreeSerifBold24pt7b );
        tft.setTextColor( goldD );
        tft.drawString( tempStr, ax + 1, 79 );
        tft.setTextColor( gold );
        tft.drawString( tempStr, ax, 78 );
        int twT = tft.textWidth( tempStr );
        drawDegreeCircle( ax + twT / 2 + 5, 66, 3, gold );

        ornament( 108 );

        // Big gold humidity, droplet right of the digits at mid-height
        String rhStr = String( currentHumidity ) + "%";
        tft.setFreeFont( &FreeSerifBold18pt7b );
        tft.setTextColor( goldD );
        tft.drawString( rhStr, ax + 1, 131 );
        tft.setTextColor( gold );
        tft.drawString( rhStr, ax, 130 );
        int rhW = tft.textWidth( rhStr );

        // Small brass water droplet beside the humidity number
        int dropX = ax + rhW / 2 + 9, dropY = 130;
        tft.fillCircle( dropX, dropY, 5, getBrassColor() );
        tft.fillTriangle( dropX - 4, dropY - 3, dropX + 4, dropY - 3, dropX, dropY - 10, getBrassColor() );
        tft.drawCircle( dropX, dropY, 5, getBrassDarkColor() );
        tft.drawLine( dropX - 3, dropY - 4, dropX, dropY - 10, getBrassDarkColor() );
        tft.drawLine( dropX + 3, dropY - 4, dropX, dropY - 10, getBrassDarkColor() );

        ornament( 152 );

        // Stacked date: SAT / 23 / JUN — uniform 12pt, wide even spacing
        struct tm ti;
        if ( getLocalTime( &ti ) ) {
            const char *wd[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
            const char *mo[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                 "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
            tft.setTextDatum( MC_DATUM );
            tft.setFreeFont( &FreeSerifBold12pt7b );
            tft.setTextColor( gold );
            tft.drawString( wd[ ti.tm_wday ], ax, 166 );
            tft.drawString( String( ti.tm_mday ), ax, 192 );
            tft.drawString( mo[ ti.tm_mon ], ax, 218 );
        }
        tft.setFreeFont( NULL );

        drawWoodFrame();   // restore frame edge over the repainted panel
        return;
    }

    // --- 1. Current temperature with icon ---
    drawWeatherIconVector( weatherCode, 5, 15 );
    tft.setTextDatum( TL_DATUM );
    tft.setTextColor( txtContrast, bg );
    tft.setFreeFont( &FreeSansBold18pt7b );

    float dispTemp = weatherUnitF ? ( currentTemp * 9.0 / 5.0 + 32 ) : currentTemp;
    String unit = weatherUnitF ? "F" : "C";
    String tempStr = String( ( int )dispTemp );
    cjkDraw( tempStr, 45, 15 );

    int tempWidth = tft.textWidth( tempStr );
    drawDegreeCircle( 45 + tempWidth + 5, 20, 3, txtContrast );
    cjkDraw( unit, 45 + tempWidth + 12, 15 );

    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextColor( txt, bg );
    cjkDraw( getWeatherDesc( weatherCode ), 45, 48 );

    tft.setFreeFont( NULL );
    tft.setTextColor( txt, bg );
    tft.setCursor( 5, 75 );
    if ( weatherUnitInHg ) {
        float pressInHg = currentPressure * 0.02953f;
        tft.printf( "RH: %d%%  P: %.2f inHg", currentHumidity, pressInHg );
    }
    else {
        tft.printf( "RH: %d%%  P: %d hPa", currentHumidity, currentPressure );
    }

    tft.setCursor( 5, 88 );
    if ( weatherUnitMph ) {
        float windMph = currentWindSpeed * 0.621371;
        tft.printf( "Wind: %.1f mph %s", windMph, getWindDir( currentWindDirection ).c_str() );
    }
    else {
        tft.printf( "Wind: %.1f km/h %s", currentWindSpeed, getWindDir( currentWindDirection ).c_str() );
    }

    // --- Sunrise/Sunset ---
    tft.drawBitmap( 5, 98, icon_sunrise, 16, 16, TFT_ORANGE );
    tft.setCursor( 24, 102 );
    tft.setTextColor( TFT_ORANGE, bg );
    tft.print( sunriseTime );

    tft.drawBitmap( 85, 101, icon_sunset, 16, 16, TFT_RED );
    tft.setCursor( 104, 102 );
    tft.setTextColor( TFT_RED, bg );
    tft.print( sunsetTime );

    tft.setTextColor( txt, bg );
    tft.drawFastHLine( 5, 120, 145, lineColor );

    // --- 2. Forecast ---
    tft.setTextColor( txt, bg );
    tft.setFreeFont( NULL );
    cjkDraw( "预报:", 5, 128 );

    drawWeatherIconVectorSmall( forecast[ 0 ].code, 8, 138 );
    tft.setTextDatum( ML_DATUM );
    tft.setFreeFont( NULL );
    tft.setTextColor( txt, bg );
    int day1x = 70;
    int day1y = 138;
    cjkDraw( forecastDay1Name, day1x, day1y );

    tft.setTextColor( txtContrast, bg );
    float fMin1 = weatherUnitF ? ( forecast[ 0 ].tempMin * 9.0 / 5.0 + 32 ) : forecast[ 0 ].tempMin;
    float fMax1 = weatherUnitF ? ( forecast[ 0 ].tempMax * 9.0 / 5.0 + 32 ) : forecast[ 0 ].tempMax;
    String tempMin1 = String( ( int )fMin1 );
    String tempMax1 = String( ( int )fMax1 );
    String tempRangeOnly1 = tempMin1 + "/" + tempMax1;
    cjkDraw( tempRangeOnly1, day1x, day1y + 13 );
    int tempWidth1 = tft.textWidth( tempRangeOnly1 );
    int degreeX1 = day1x + tempWidth1 + 3;
    int degreeY1 = day1y + 8;
    drawDegreeCircle( degreeX1, degreeY1, 1, txtContrast );
    cjkDraw( unit, degreeX1 + 4, day1y + 13 );

    drawWeatherIconVectorSmall( forecast[ 1 ].code, 8, 170 );
    tft.setTextColor( txt, bg );
    int day2x = 70;
    int day2y = 170;
    cjkDraw( forecastDay2Name, day2x, day2y );

    tft.setTextColor( txtContrast, bg );
    float fMin2 = weatherUnitF ? ( forecast[ 1 ].tempMin * 9.0 / 5.0 + 32 ) : forecast[ 1 ].tempMin;
    float fMax2 = weatherUnitF ? ( forecast[ 1 ].tempMax * 9.0 / 5.0 + 32 ) : forecast[ 1 ].tempMax;
    String tempMin2 = String( ( int )fMin2 );
    String tempMax2 = String( ( int )fMax2 );
    String tempRangeOnly2 = tempMin2 + "/" + tempMax2;
    cjkDraw( tempRangeOnly2, day2x, day2y + 13 );
    int tempWidth2 = tft.textWidth( tempRangeOnly2 );
    int degreeX2 = day2x + tempWidth2 + 3;
    int degreeY2 = day2y + 8;
    drawDegreeCircle( degreeX2, degreeY2, 1, txtContrast );
    cjkDraw( unit, degreeX2 + 4, day2y + 13 );

    tft.setTextColor( txt, bg );
    tft.drawFastHLine( 5, 200, 145, lineColor );

    // --- 3. Moon phase ---
    struct tm ti;
    if ( getLocalTime( &ti ) ) {
        int phase = getMoonPhase( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday );
        moonPhaseVal = phase;

        tft.setTextColor( txt, bg );
        tft.setFreeFont( NULL );
        cjkDraw( "月相:", 5, 210 );

        String phaseNames[] = {"新月", "娥眉月", "上弦月", "盈凸月", "满月", "亏凸月", "下弦月", "残月"};
        if ( phase >= 0 && phase <= 7 ) {
            cjkDraw( phaseNames[ phase ], 5, 222 );
        }

        int mx = 120;
        int my = 222;
        int r = 13;
        drawMoonPhaseIcon( mx, my, r, phase, txt, bg );

        log_d( "[MOON] Phase: %d | Date: %d-%d-%d", phase, ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday );
    }
}
