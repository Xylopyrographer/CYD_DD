#include "clock_face.h"
#include "theme.h"
#include "icons.h"

#include <TFT_eSPI.h>
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
// Clock-face sprite  — 140×140 px, rendered off-screen then pushed atomically
// so no intermediate state (tick erase → ticks redraw → hand redraw) is visible.
// ---------------------------------------------------------------------------
static TFT_eSprite clockSprite( &tft );
static bool        spriteCreated = false;
static int         spriteX = 0, spriteY = 0;  // top-left of sprite on screen
static int         sCX = 0,     sCY = 0;       // clock centre in sprite coords

static void createClockSprite() {
    if ( spriteCreated ) {
        return;
    }
    int sz  = ( radius + 3 ) * 2;          // e.g. 140 for radius=67
    spriteX = clockX - ( radius + 3 );     // 160
    spriteY = clockY - ( radius + 3 );     // 15
    sCX     = radius + 3;                  // 70  (centre inside sprite)
    sCY     = radius + 3;                  // 70
    clockSprite.setColorDepth( 16 );
    clockSprite.createSprite( sz, sz );
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
        tft.drawString( String( h ), x, y );
    }
}

void drawClockFace() {
    tft.fillScreen( getBgColor() );
    if ( !isDigitalClock ) {
        createClockSprite();   // allocate sprite (no-op if already done)
        tft.drawCircle( clockX, clockY, radius + 2, getTextColor() );
        drawClockStatic();
    }
    forceClockRedraw = true;
}

void drawDateAndWeek( const struct tm *ti ) {
    uint16_t dateColor = getTextColor();
    if ( themeMode == THEME_YELLOW ) {
        dateColor = TFT_BLACK;
    }

    tft.setFreeFont( NULL );
    tft.setTextColor( dateColor, getBgColor() );
    tft.setTextDatum( MC_DATUM );

    tft.fillRect( 155, 160, 165, 80, getBgColor() );

    char dateBuf[ 30 ];
    strftime( dateBuf, sizeof( dateBuf ), "%B %d, %Y", ti );
    tft.drawString( String( dateBuf ), clockX, 175, 2 );

    int weekNum = 0;
    char weekBuf[ 20 ];
    strftime( weekBuf, sizeof( weekBuf ), "%V", ti );
    weekNum = atoi( weekBuf );

    const char *dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    String dayStr = String( dayNames[ ti->tm_wday ] );
    String weekStr = "Week " + String( weekNum ) + ", " + dayStr;
    tft.drawString( weekStr, clockX, 193, 2 );

    if ( cityName != "" ) {
        if ( themeMode == THEME_YELLOW ) {
            tft.setTextColor( 0x0220, getBgColor() ); // Dark green
        }
        else {
            tft.setTextColor( TFT_SKYBLUE, getBgColor() );
        }
        tft.drawString( cityName, clockX, 211, 2 );
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
        tft.drawString( todayHoliday, clockX, 227, 1 );
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
        tft.drawString( "Nameday: " + todayNameday, clockX, 227, 1 );
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
    const char *suffix = "";

    if ( is12hFormat ) {
        if ( displayH >= 12 ) {
            suffix = " PM";
            if ( displayH > 12 ) {
                displayH -= 12;
            }
        }
        else {
            suffix = " AM";
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

    tft.setTextDatum( MC_DATUM );
    tft.setTextColor( clockColor, bgColor );

    tft.drawString( timeStr, clockX, clockY, 7 );

    char secStr[ 10 ];
    if ( is12hFormat ) {
        sprintf( secStr, ":%02d%s", s, suffix );
    }
    else {
        sprintf( secStr, ":%02d", s );
    }

    tft.setTextColor( getSecHandColor(), bgColor );
    tft.drawString( secStr, clockX, clockY + 45, 4 );
}

void updateHands( int h, int m, int s ) {
    if ( isDigitalClock ) {
        drawDigitalClock( h, m, s );
        return;
    }

    createClockSprite();   // no-op after first call; safe even if drawClockFace() skipped

    uint16_t bgColor       = getBgColor();
    uint16_t mainHandColor = getTextColor();
    uint16_t secColor      = getSecHandColor();

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

    // Hands
    float hA = ( ( h % 12 ) + ( m / 60.0f ) ) * 30.0f - 90.0f;
    float mA = m * 6.0f  - 90.0f;
    float sA = s * 6.0f  - 90.0f;

    clockSprite.drawLine( sCX, sCY,
                          sCX + ( int )( cos( hA * DEGTORAD_CF ) * ( radius - 35 ) ),
                          sCY + ( int )( sin( hA * DEGTORAD_CF ) * ( radius - 35 ) ),
                          mainHandColor );
    clockSprite.drawLine( sCX, sCY,
                          sCX + ( int )( cos( mA * DEGTORAD_CF ) * ( radius - 20 ) ),
                          sCY + ( int )( sin( mA * DEGTORAD_CF ) * ( radius - 20 ) ),
                          mainHandColor );
    clockSprite.drawLine( sCX, sCY,
                          sCX + ( int )( cos( sA * DEGTORAD_CF ) * ( radius - 14 ) ),
                          sCY + ( int )( sin( sA * DEGTORAD_CF ) * ( radius - 14 ) ),
                          secColor );
    clockSprite.fillCircle( sCX, sCY, 3, TFT_LIGHTGREY );

    // Push sprite — single SPI burst, no intermediate state on screen
    clockSprite.pushSprite( spriteX, spriteY );
}

void drawWeatherSection() {
    uint16_t bg = getBgColor();
    uint16_t txt = getTextColor();
    uint16_t txtContrast = TFT_SKYBLUE;

    if ( themeMode == THEME_BLUE ) {
        txtContrast = TFT_YELLOW;
    }
    else if ( themeMode == THEME_YELLOW ) {
        txtContrast = TFT_BLACK;
    }

    tft.fillRect( 0, 0, 155, 95, bg );
    tft.fillRect( 0, 105, 155, 100, bg );
    tft.fillRect( 0, 206, 155, 34, bg );

    if ( !initialWeatherFetched ) {
        tft.setTextColor( txt, bg );
        tft.setTextDatum( MC_DATUM );
        tft.drawString( "Loading...", 75, 120 );
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
    tft.drawString( tempStr, 45, 15 );

    int tempWidth = tft.textWidth( tempStr );
    drawDegreeCircle( 45 + tempWidth + 5, 20, 3, txtContrast );
    tft.drawString( unit, 45 + tempWidth + 12, 15 );

    tft.setFreeFont( &FreeSans9pt7b );
    tft.setTextColor( txt, bg );
    tft.drawString( getWeatherDesc( weatherCode ), 45, 48 );

    tft.setFreeFont( NULL );
    tft.setTextColor( txt, bg );
    tft.setCursor( 5, 75 );
    if ( weatherUnitInHg ) {
        float pressInHg = currentPressure * 0.02953f;
        tft.printf( "Hum: %d%% Press: %.2f inHg", currentHumidity, pressInHg );
    }
    else {
        tft.printf( "Hum: %d%% Press: %d hPa", currentHumidity, currentPressure );
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
    tft.drawFastHLine( 5, 120, 145, TFT_DARKGREY );

    // --- 2. Forecast ---
    tft.setTextColor( txt, bg );
    tft.setFreeFont( NULL );
    tft.drawString( "Forecast:", 5, 128 );

    drawWeatherIconVectorSmall( forecast[ 0 ].code, 8, 138 );
    tft.setTextDatum( ML_DATUM );
    tft.setFreeFont( NULL );
    tft.setTextColor( txt, bg );
    int day1x = 70;
    int day1y = 138;
    tft.drawString( forecastDay1Name, day1x, day1y );

    tft.setTextColor( txtContrast, bg );
    float fMin1 = weatherUnitF ? ( forecast[ 0 ].tempMin * 9.0 / 5.0 + 32 ) : forecast[ 0 ].tempMin;
    float fMax1 = weatherUnitF ? ( forecast[ 0 ].tempMax * 9.0 / 5.0 + 32 ) : forecast[ 0 ].tempMax;
    String tempMin1 = String( ( int )fMin1 );
    String tempMax1 = String( ( int )fMax1 );
    String tempRangeOnly1 = tempMin1 + "/" + tempMax1;
    tft.drawString( tempRangeOnly1, day1x, day1y + 13 );
    int tempWidth1 = tft.textWidth( tempRangeOnly1 );
    int degreeX1 = day1x + tempWidth1 + 3;
    int degreeY1 = day1y + 8;
    drawDegreeCircle( degreeX1, degreeY1, 1, txtContrast );
    tft.drawString( unit, degreeX1 + 4, day1y + 13 );

    drawWeatherIconVectorSmall( forecast[ 1 ].code, 8, 170 );
    tft.setTextColor( txt, bg );
    int day2x = 70;
    int day2y = 170;
    tft.drawString( forecastDay2Name, day2x, day2y );

    tft.setTextColor( txtContrast, bg );
    float fMin2 = weatherUnitF ? ( forecast[ 1 ].tempMin * 9.0 / 5.0 + 32 ) : forecast[ 1 ].tempMin;
    float fMax2 = weatherUnitF ? ( forecast[ 1 ].tempMax * 9.0 / 5.0 + 32 ) : forecast[ 1 ].tempMax;
    String tempMin2 = String( ( int )fMin2 );
    String tempMax2 = String( ( int )fMax2 );
    String tempRangeOnly2 = tempMin2 + "/" + tempMax2;
    tft.drawString( tempRangeOnly2, day2x, day2y + 13 );
    int tempWidth2 = tft.textWidth( tempRangeOnly2 );
    int degreeX2 = day2x + tempWidth2 + 3;
    int degreeY2 = day2y + 8;
    drawDegreeCircle( degreeX2, degreeY2, 1, txtContrast );
    tft.drawString( unit, degreeX2 + 4, day2y + 13 );

    tft.setTextColor( txt, bg );
    tft.drawFastHLine( 5, 200, 145, TFT_DARKGREY );

    // --- 3. Moon phase ---
    struct tm ti;
    if ( getLocalTime( &ti ) ) {
        int phase = getMoonPhase( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday );
        moonPhaseVal = phase;

        tft.setTextColor( txt, bg );
        tft.setFreeFont( NULL );
        tft.drawString( "Moon Phase:", 5, 210 );

        String phaseNames[] = {"New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous", "Full Moon", "Waning Gibbous", "Last Quarter", "Waning Crescent"};
        if ( phase >= 0 && phase <= 7 ) {
            tft.drawString( phaseNames[ phase ], 5, 222 );
        }

        int mx = 120;
        int my = 222;
        int r = 13;
        drawMoonPhaseIcon( mx, my, r, phase, txt, bg );

        log_d( "[MOON] Phase: %d | Date: %d-%d-%d", phase, ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday );
    }
}
