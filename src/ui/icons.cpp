#include "icons.h"
#include "theme.h"
#include "../util/constants.h"

#include <WiFi.h>
#include <TFT_eSPI.h>

// ---------------------------------------------------------------------------
// Externs – defined in main.cpp
// ---------------------------------------------------------------------------
extern TFT_eSPI  tft;
extern bool      isWhiteTheme;
extern int       themeMode;
extern uint16_t  blueDark;
extern uint16_t  yellowDark;
extern bool      updateAvailable;

static constexpr float DEGTORAD_ICONS = ( float )( PI / 180.0 );

// ---------------------------------------------------------------------------

void drawCloudVector( int x, int y, uint32_t color ) {
    tft.fillCircle( x + 10, y + 15, 8, color );
    tft.fillCircle( x + 18, y + 10, 10, color );
    tft.fillCircle( x + 28, y + 15, 8, color );
    tft.fillRoundRect( x + 10, y + 15, 20, 8, 4, color );
}

void drawWeatherIconVector( int code, int x, int y ) {
    // Icon colors adapt to the theme
    uint16_t cloudCol = TFT_SILVER;
    uint16_t shadowCol = isWhiteTheme ? 0x8410 : 0x4208; // Shadow in blue/yellow theme

    switch ( code ) {
        case 0: // Clear
            // Sun with shadow
            tft.fillCircle( x + 16, y + 16, 10, TFT_YELLOW );
            tft.drawCircle( x + 16, y + 16, 11, shadowCol ); // Shadow
            for ( int i = 0; i < 360; i += 45 ) {
                float rad = i * 0.01745;
                tft.drawLine( x + 16 + cos( rad ) * 11, y + 16 + sin( rad ) * 11, x + 16 + cos( rad ) * 16, y + 16 + sin( rad ) * 16, TFT_YELLOW );
            }
            break;

        case 1:
        case 2:
        case 3: // Partly cloudy
            tft.fillCircle( x + 22, y + 10, 8, TFT_YELLOW );
            tft.drawCircle( x + 22, y + 10, 9, shadowCol ); // Shadow
            drawCloudVector( x, y + 5, cloudCol );
            break;

        case 45:
        case 48: // Fog
            for ( int i = 0; i < 3; i++ ) {
                tft.fillRoundRect( x + 4, y + 12 + ( i * 6 ), 24, 3, 2, TFT_SILVER );
                tft.drawRoundRect( x + 4, y + 12 + ( i * 6 ), 24, 3, 2, shadowCol ); // Shadow
            }
            break;

        case 51:
        case 53:
        case 55:
        case 61:
        case 63:
        case 65: // Rain
            drawCloudVector( x, y + 2, TFT_SILVER );
            for ( int i = 0; i < 3; i++ ) {
                tft.fillRoundRect( x + 10 + ( i * 6 ), y + 22, 2, 6, 1, TFT_BLUE );
                tft.drawRoundRect( x + 10 + ( i * 6 ), y + 22, 2, 6, 1, shadowCol ); // Drop shadow
            }
            break;

        case 71:
        case 73:
        case 75:
        case 77: // Snow
            drawCloudVector( x, y + 2, cloudCol );
            tft.setTextColor( TFT_SKYBLUE );
            tft.drawString( "*", x + 12, y + 22 );
            tft.drawString( "*", x + 22, y + 22 );
            break;

        case 80:
        case 81:
        case 82: // Showers
            tft.fillCircle( x + 22, y + 10, 7, TFT_YELLOW );
            tft.drawCircle( x + 22, y + 10, 8, shadowCol ); // Shadow
            drawCloudVector( x, y + 2, TFT_SILVER );
            tft.fillRoundRect( x + 16, y + 22, 2, 6, 1, TFT_BLUE );
            break;

        case 95:
        case 96:
        case 99: // Storm
            drawCloudVector( x, y + 2, shadowCol ); // Dark cloud
            tft.drawLine( x + 18, y + 20, x + 14, y + 28, TFT_YELLOW );
            tft.drawLine( x + 14, y + 28, x + 20, y + 28, TFT_YELLOW );
            tft.drawLine( x + 20, y + 28, x + 16, y + 36, TFT_YELLOW );
            break;

        default:
            drawCloudVector( x, y + 5, TFT_SILVER );
            break;
    }
}

// ============================================
// NEW FEATURE: Reduced icons for forecast
// ============================================
void drawWeatherIconVectorSmall( int code, int x, int y ) {
    // A scaled-down version for forecast, but with better proportions
    uint16_t cloudCol = TFT_SILVER;
    uint16_t shadowCol = isWhiteTheme ? 0x8410 : 0x4208;

    switch ( code ) {
        case 0: // Clear
            tft.fillCircle( x + 16, y + 16, 9, TFT_YELLOW );
            tft.drawCircle( x + 16, y + 16, 10, shadowCol );
            for ( int i = 0; i < 360; i += 45 ) {
                float rad = i * 0.01745;
                tft.drawLine( x + 16 + cos( rad ) * 10, y + 16 + sin( rad ) * 10, x + 16 + cos( rad ) * 14, y + 16 + sin( rad ) * 14, TFT_YELLOW );
            }
            break;

        case 1:
        case 2:
        case 3: // Partly cloudy
            tft.fillCircle( x + 20, y + 10, 7, TFT_YELLOW );
            tft.drawCircle( x + 20, y + 10, 8, shadowCol );
            tft.fillCircle( x + 8, y + 14, 6, cloudCol );
            tft.fillCircle( x + 14, y + 11, 7, cloudCol );
            tft.fillCircle( x + 20, y + 14, 5, cloudCol );
            tft.fillRoundRect( x + 8, y + 14, 15, 5, 2, cloudCol );
            break;

        case 45:
        case 48: // Fog
            for ( int i = 0; i < 3; i++ ) {
                tft.fillRoundRect( x + 4, y + 12 + ( i * 5 ), 20, 2, 1, TFT_SILVER );
                tft.drawRoundRect( x + 4, y + 12 + ( i * 5 ), 20, 2, 1, shadowCol );
            }
            break;

        case 51:
        case 53:
        case 55:
        case 61:
        case 63:
        case 65: // Rain
            tft.fillCircle( x + 9, y + 13, 6, cloudCol );
            tft.fillCircle( x + 15, y + 10, 8, cloudCol );
            tft.fillCircle( x + 22, y + 13, 6, cloudCol );
            tft.fillRoundRect( x + 9, y + 13, 16, 6, 3, cloudCol );
            for ( int i = 0; i < 3; i++ ) {
                tft.fillRoundRect( x + 10 + ( i * 5 ), y + 21, 2, 5, 1, TFT_BLUE );
                tft.drawRoundRect( x + 10 + ( i * 5 ), y + 21, 2, 5, 1, shadowCol );
            }
            break;

        case 71:
        case 73:
        case 75:
        case 77: // Snow
            tft.fillCircle( x + 9, y + 13, 6, cloudCol );
            tft.fillCircle( x + 15, y + 10, 8, cloudCol );
            tft.fillCircle( x + 22, y + 13, 6, cloudCol );
            tft.fillRoundRect( x + 9, y + 13, 16, 6, 3, cloudCol );
            tft.setTextColor( TFT_SKYBLUE );
            tft.drawString( "*", x + 11, y + 21 );
            tft.drawString( "*", x + 19, y + 21 );
            break;

        case 80:
        case 81:
        case 82: // Showers
            tft.fillCircle( x + 20, y + 10, 7, TFT_YELLOW );
            tft.drawCircle( x + 20, y + 10, 8, shadowCol );
            tft.fillCircle( x + 8, y + 14, 6, cloudCol );
            tft.fillCircle( x + 14, y + 11, 7, cloudCol );
            tft.fillCircle( x + 20, y + 14, 5, cloudCol );
            tft.fillRoundRect( x + 8, y + 14, 15, 5, 2, cloudCol );
            tft.fillRoundRect( x + 14, y + 21, 2, 5, 1, TFT_BLUE );
            break;

        case 95:
        case 96:
        case 99: // Storm
            tft.fillCircle( x + 9, y + 13, 6, shadowCol );
            tft.fillCircle( x + 15, y + 10, 8, shadowCol );
            tft.fillCircle( x + 22, y + 13, 6, shadowCol );
            tft.fillRoundRect( x + 9, y + 13, 16, 6, 3, shadowCol );
            tft.drawLine( x + 15, y + 20, x + 12, y + 27, TFT_YELLOW );
            tft.drawLine( x + 12, y + 27, x + 17, y + 27, TFT_YELLOW );
            tft.drawLine( x + 17, y + 27, x + 14, y + 34, TFT_YELLOW );
            break;

        default:
            tft.fillCircle( x + 9, y + 13, 6, cloudCol );
            tft.fillCircle( x + 15, y + 10, 8, cloudCol );
            tft.fillCircle( x + 22, y + 13, 6, cloudCol );
            tft.fillRoundRect( x + 9, y + 13, 16, 6, 3, cloudCol );
            break;
    }
}

// ============================================
// NEW FEATURE FOR CORRECT DRAWING OF MOON PHASE
// ============================================
void drawMoonPhaseIcon( int mx, int my, int r, int phase, uint16_t textColor, uint16_t bgColor ) {
    uint16_t moonBg = ( themeMode == THEME_BLUE ) ? blueDark : ( themeMode == THEME_YELLOW ) ? yellowDark : ( isWhiteTheme ? 0xDEDB : 0x3186 );
    uint16_t moonColor = TFT_YELLOW;
    uint16_t shadowColor = moonBg;

    tft.drawCircle( mx, my, r, textColor );

    switch ( phase ) {

        case 0: { // NEW MOON
            tft.fillCircle( mx, my, r - 1, shadowColor );
            break;
        }

        case 1: { // WAXING CRESCENT
            tft.fillCircle( mx, my, r - 1, shadowColor );
            int offset = r / 3;
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                int light_boundary = sqrt( r * r - dy * dy - offset * offset ) - offset;
                if ( light_boundary < 0 ) {
                    light_boundary = 0;
                }
                for ( int dx = light_boundary; dx <= dx_max; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, moonColor );
                }
            }
            break;
        }

        case 2: { // FIRST QUARTER
            tft.fillCircle( mx, my, r - 1, shadowColor );
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                for ( int dx = 0; dx <= dx_max; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, moonColor );
                }
            }
            break;
        }

        case 3: { // WAXING GIBBOUS
            tft.fillCircle( mx, my, r - 1, moonColor );
            int offset = r / 3;
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                int shadow_boundary = -( sqrt( r * r - dy * dy - offset * offset ) - offset );
                if ( shadow_boundary > 0 ) {
                    shadow_boundary = 0;
                }
                for ( int dx = -dx_max; dx <= shadow_boundary; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, shadowColor );
                }
            }
            break;
        }

        case 4: { // FULL MOON
            tft.fillCircle( mx, my, r - 1, moonColor );
            break;
        }

        case 5: { // WANING GIBBOUS
            tft.fillCircle( mx, my, r - 1, moonColor );
            int offset = r / 3;
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                int shadow_boundary = sqrt( r * r - dy * dy - offset * offset ) - offset;
                if ( shadow_boundary < 0 ) {
                    shadow_boundary = 0;
                }
                for ( int dx = shadow_boundary; dx <= dx_max; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, shadowColor );
                }
            }
            break;
        }

        case 6: { // LAST QUARTER
            tft.fillCircle( mx, my, r - 1, shadowColor );
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                for ( int dx = -dx_max; dx <= 0; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, moonColor );
                }
            }
            break;
        }

        case 7: { // WANING CRESCENT
            tft.fillCircle( mx, my, r - 1, shadowColor );
            int offset = r / 3;
            for ( int dy = -r; dy <= r; dy++ ) {
                int dx_max = sqrt( r * r - dy * dy );
                int light_boundary = -( sqrt( r * r - dy * dy - offset * offset ) - offset );
                if ( light_boundary > 0 ) {
                    light_boundary = 0;
                }
                for ( int dx = -dx_max; dx <= light_boundary; dx++ ) {
                    tft.drawPixel( mx + dx, my + dy, moonColor );
                }
            }
            break;
        }

        default: {
            tft.drawCircle( mx, my, r, textColor );
            break;
        }
    }
}

void drawWifiIndicator() {
    int wifiStatus = WiFi.status();
    uint16_t color = wifiStatus == WL_CONNECTED ? TFT_GREEN : TFT_RED;
    tft.fillCircle( 305, 20, 4, color );
}

// Update available indicator — upward triangle + stem, offset 4 px right of WiFi circle
void drawUpdateIndicator() {
    if ( !updateAvailable ) {
        return;
    }

    // Erase previous icon footprint before drawing (handles redraws without a prior fillScreen)
    tft.fillRect( 310, 14, 10, 12, getBgColor() );

    int iconX = 313;  // 4 px gap from WiFi circle right edge (x=309)
    int iconY = 15;   // Centres the 10px-tall icon at y=20 (matching WiFi circle centre)

    // Upward-pointing filled triangle (6 px wide, 5 px tall — less pointy)
    tft.fillTriangle( iconX, iconY + 5, iconX + 3, iconY, iconX + 6, iconY + 5, TFT_GREEN );
    // Stem extending down from triangle base (centred, 2 px wide, 5 px tall)
    tft.fillRect( iconX + 2, iconY + 5, 2, 5, TFT_GREEN );
}

void drawSettingsIcon( uint16_t color ) {
    int ix = 300, iy = 220;
    int rIn = 3, rMid = 6, rOut = 8;
    tft.fillCircle( ix, iy, rMid, color );
    tft.fillCircle( ix, iy, rIn, getBgColor() );
    for ( int i = 0; i < 8; i++ ) {
        float a = i * 45 * DEGTORAD_ICONS;
        float aL = a - 0.2;
        float aR = a + 0.2;
        tft.fillTriangle( ix + cos( aL ) * rMid, iy + sin( aL ) * rMid, ix + cos( aR ) * rMid, iy + sin( aR ) * rMid, ix + cos( a ) * rOut, iy + sin( a ) * rOut, color );
    }
}

void drawArrowBack( int x, int y, uint16_t color ) {
    tft.drawRoundRect( x, y, 50, 50, 4, color );
    tft.drawLine( x + 35, y + 15, x + 20, y + 25, color );
    tft.drawLine( x + 35, y + 35, x + 20, y + 25, color );
    tft.drawLine( x + 34, y + 15, x + 19, y + 25, color );
    tft.drawLine( x + 34, y + 35, x + 19, y + 25, color );
}

void drawArrowDown( int x, int y, uint16_t color ) {
    tft.drawRoundRect( x, y, 50, 50, 4, color );
    tft.drawLine( x + 15, y + 20, x + 25, y + 35, color );
    tft.drawLine( x + 35, y + 20, x + 25, y + 35, color );
    tft.drawLine( x + 15, y + 21, x + 25, y + 36, color );
    tft.drawLine( x + 35, y + 21, x + 25, y + 36, color );
}

void drawArrowUp( int x, int y, uint16_t color ) {
    tft.drawRoundRect( x, y, 50, 50, 4, color );
    tft.drawLine( x + 15, y + 35, x + 25, y + 20, color );
    tft.drawLine( x + 35, y + 35, x + 25, y + 20, color );
    tft.drawLine( x + 15, y + 34, x + 25, y + 19, color );
    tft.drawLine( x + 35, y + 34, x + 25, y + 19, color );
}

void fillGradientVertical( int x, int y, int w, int h, uint16_t colorTop, uint16_t colorBottom ) {
    for ( int i = 0; i < h; i++ ) {
        uint8_t r1 = ( colorTop >> 11 ) & 0x1F;
        uint8_t g1 = ( colorTop >> 5 ) & 0x3F;
        uint8_t b1 = colorTop & 0x1F;

        uint8_t r2 = ( colorBottom >> 11 ) & 0x1F;
        uint8_t g2 = ( colorBottom >> 5 ) & 0x3F;
        uint8_t b2 = colorBottom & 0x1F;

        float ratio = ( float )i / h;
        uint8_t r = r1 + ( r2 - r1 ) * ratio;
        uint8_t g = g1 + ( g2 - g1 ) * ratio;
        uint8_t b = b1 + ( b2 - b1 ) * ratio;

        uint16_t color = ( r << 11 ) | ( g << 5 ) | b;
        tft.drawFastHLine( x, y + i, w, color );
    }
}

// ============================================
// HELPER FUNCTION FOR DRAWING DEGREE SYMBOL
// ============================================
void drawDegreeCircle( int x, int y, int r, uint16_t color ) {
    tft.drawCircle( x, y, r, color );
    if ( r > 1 ) {
        tft.drawCircle( x, y, r - 1, color );
    }
}

