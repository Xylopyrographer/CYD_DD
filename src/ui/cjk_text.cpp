#include "cjk_text.h"
#include "cjk_font.h"

extern TFT_eSPI tft;

namespace {

// Map a tft.drawString() font number to a CJK bitmap font and pixel scale.
//  0 (FreeFont / unspecified) -> 16 px
//  1 (8 px GLCD)              -> 12 px
//  2 (16 px)                  -> 16 px
//  4 (26 px)                  -> 16 px at 2x (32 px, headings)
// Anything else falls back to 16 px.
const CjkFont *pickFont( uint8_t font, int &scale )
{
    scale = 1;
    if ( font == 1 ) return &CJK_FONT_12;
    if ( font == 4 ) {
        scale = 2;
        return &CJK_FONT_16;
    }
    return &CJK_FONT_16;
}

const CjkGlyph *findGlyph( const CjkFont *f, uint16_t cp )
{
    int lo = 0, hi = f->count - 1;
    while ( lo <= hi ) {
        int mid = ( lo + hi ) >> 1;
        if ( f->glyphs[ mid ].cp == cp ) return &f->glyphs[ mid ];
        if ( f->glyphs[ mid ].cp < cp ) lo = mid + 1;
        else hi = mid - 1;
    }
    return nullptr;
}

// Decode one UTF-8 codepoint, return bytes consumed (0 on end / error)
int utf8Next( const char *s, uint16_t &cp )
{
    uint8_t c = (uint8_t)*s;
    if ( c < 0x80 ) {
        cp = c;
        return c ? 1 : 0;
    }
    int n = 0;
    uint32_t v = 0;
    if ( ( c & 0xE0 ) == 0xC0 ) {
        n = 2;
        v = c & 0x1F;
    }
    else if ( ( c & 0xF0 ) == 0xE0 ) {
        n = 3;
        v = c & 0x0F;
    }
    else if ( ( c & 0xF8 ) == 0xF0 ) {
        n = 4;
        v = c & 0x07;
    }
    else {
        cp = '?';
        return 1;
    }
    for ( int i = 1; i < n; i++ ) {
        uint8_t cc = (uint8_t)s[ i ];
        if ( ( cc & 0xC0 ) != 0x80 ) {
            cp = '?';
            return 1;
        }
        v = ( v << 6 ) | ( cc & 0x3F );
    }
    cp = (uint16_t)v;   // UI glyphs live in the BMP
    return n;
}

bool isPureAscii( const String &s )
{
    for ( size_t i = 0; i < s.length(); i++ )
        if ( (uint8_t)s[ i ] > 126 ) return false;
    return true;
}

TFT_eSprite *sprite = nullptr;
int spriteW = 0, spriteH = 0;

void ensureSprite( int w, int h )
{
    if ( sprite && w <= spriteW && h <= spriteH ) return;
    if ( sprite ) {
        sprite->deleteSprite();
        delete sprite;
        sprite = nullptr;
    }
    sprite = new TFT_eSprite( &tft );
    uint16_t *buf = (uint16_t *)sprite->createSprite( w, h );
    if ( !buf ) {   // out of heap: fall back to direct pixel drawing
        sprite->deleteSprite();
        delete sprite;
        sprite = nullptr;
        spriteW = spriteH = 0;
        return;
    }
    spriteW = w;
    spriteH = h;
}

}   // namespace

int cjkMeasure( const String &text, uint8_t font )
{
    if ( isPureAscii( text ) ) return tft.textWidth( text, font );
    int scale;
    const CjkFont *f = pickFont( font, scale );
    int w = 0;
    const char *p = text.c_str();
    while ( *p ) {
        uint16_t cp;
        int n = utf8Next( p, cp );
        if ( !n ) break;
        p += n;
        const CjkGlyph *g = findGlyph( f, cp );
        w += ( g ? g->adv : f->height / 2 ) * scale;
    }
    return w;
}

void cjkDraw( const String &text, int32_t x, int32_t y, uint8_t font )
{
    if ( isPureAscii( text ) ) {
        tft.drawString( text, x, y, font );
        return;
    }

    int scale;
    const CjkFont *f = pickFont( font, scale );
    const int H = f->height * scale;
    const int W = cjkMeasure( text, font );
    if ( W <= 0 ) return;

    const uint8_t datum = tft.getTextDatum();
    int dx = x, dy = y;
    switch ( datum ) {
        case TC_DATUM: dx = x - W / 2; break;
        case TR_DATUM: dx = x - W; break;
        case ML_DATUM: dy = y - H / 2; break;
        case MC_DATUM: dx = x - W / 2; dy = y - H / 2; break;
        case MR_DATUM: dx = x - W; dy = y - H / 2; break;
        case BL_DATUM: dy = y - H; break;
        case BC_DATUM: dx = x - W / 2; dy = y - H; break;
        case BR_DATUM: dx = x - W; dy = y - H; break;
        default: break;   // TL_DATUM
    }

    const uint16_t fg = (uint16_t)tft.textcolor;
    const uint16_t trans = (uint16_t)~fg;   // guaranteed != fg

    ensureSprite( W, H );

    if ( sprite ) {
        sprite->fillSprite( trans );
        int cx = 0;
        const char *p = text.c_str();
        while ( *p && cx < W ) {
            uint16_t cp;
            int n = utf8Next( p, cp );
            if ( !n ) break;
            p += n;
            const CjkGlyph *g = findGlyph( f, cp );
            if ( g ) {
                for ( int r = 0; r < f->height; r++ ) {
                    const uint8_t *row = g->bits + r * f->rowBytes;
                    for ( int c = 0; c < g->adv; c++ ) {
                        if ( pgm_read_byte( &row[ c >> 3 ] ) & ( 0x80 >> ( c & 7 ) ) ) {
                            if ( scale == 1 ) sprite->drawPixel( cx + c, r, fg );
                            else sprite->fillRect( cx + c * 2, r * 2, 2, 2, fg );
                        }
                    }
                }
                cx += g->adv * scale;
            }
            else {
                cx += ( f->height / 2 ) * scale;   // unknown glyph placeholder
            }
        }
        sprite->pushSprite( dx, dy, trans );
    }
    else {
        // No heap for the sprite: draw set pixels straight to the TFT
        int cx = 0;
        const char *p = text.c_str();
        while ( *p && cx < W ) {
            uint16_t cp;
            int n = utf8Next( p, cp );
            if ( !n ) break;
            p += n;
            const CjkGlyph *g = findGlyph( f, cp );
            if ( g ) {
                for ( int r = 0; r < f->height; r++ ) {
                    const uint8_t *row = g->bits + r * f->rowBytes;
                    for ( int c = 0; c < g->adv; c++ ) {
                        if ( pgm_read_byte( &row[ c >> 3 ] ) & ( 0x80 >> ( c & 7 ) ) ) {
                            tft.fillRect( dx + cx + c * scale, dy + r * scale, scale, scale, fg );
                        }
                    }
                }
                cx += g->adv * scale;
            }
            else {
                cx += ( f->height / 2 ) * scale;
            }
        }
    }
}

void cjkDraw( const String &text, int32_t x, int32_t y )
{
    // ASCII goes through unchanged so FreeFont call sites keep their metrics;
    // CJK text has no bold FreeFont equivalent, render at 16 px.
    if ( isPureAscii( text ) ) {
        tft.drawString( text, x, y );
        return;
    }
    cjkDraw( text, x, y, 2 );
}
