#include "credentials.h"
#include <Arduino.h>

// ===== PASSWORD OBFUSCATION =====
// Simple XOR + hex encoding to prevent plaintext WiFi passwords in NVS.
// Not cryptographic — intended as casual obfuscation only.
// Backward compatible: non-hex or odd-length stored values are treated as legacy plaintext.
static const uint8_t PASS_XOR_KEY[] = {
    0x5A, 0x7F, 0x3C, 0xA1, 0x8E, 0x2D, 0x6B, 0xF4,
    0x19, 0xC7, 0x45, 0xBB, 0x31, 0x9E, 0x52, 0xD6
};
static const size_t PASS_XOR_KEY_LEN = sizeof( PASS_XOR_KEY );

String obfuscatePassword( const String &plain ) {
    String result;
    result.reserve( plain.length() * 2 );
    for ( size_t i = 0; i < plain.length(); i++ ) {
        uint8_t c = ( uint8_t )plain[ i ] ^ PASS_XOR_KEY[ i % PASS_XOR_KEY_LEN ];
        char hex[ 3 ];
        snprintf( hex, sizeof( hex ), "%02x", c );
        result += hex;
    }
    return result;
}

String deobfuscatePassword( const String &stored ) {
    // Detect obfuscated value: non-empty, even length, all hex digits
    if ( stored.length() == 0 ) {
        return "";
    }
    if ( stored.length() % 2 != 0 ) {
        return stored;    // Odd length → legacy plaintext
    }
    for ( size_t i = 0; i < stored.length(); i++ ) {
        if ( !isxdigit( ( unsigned char )stored[ i ] ) ) {
            return stored;    // Non-hex → legacy plaintext
        }
    }
    String result;
    size_t len = stored.length() / 2;
    result.reserve( len );
    for ( size_t i = 0; i < len; i++ ) {
        char hexByte[ 3 ] = { stored[ i * 2 ], stored[ i * 2 + 1 ], '\0' };
        uint8_t c = ( uint8_t )strtol( hexByte, nullptr, 16 );
        c ^= PASS_XOR_KEY[ i % PASS_XOR_KEY_LEN ];
        result += ( char )c;
    }
    return result;
}

