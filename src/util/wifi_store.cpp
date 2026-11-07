#include "wifi_store.h"
#include "credentials.h"

#include <Preferences.h>

// All keys live in the existing "sys" namespace so boot-time nvsInitialized
// detection (prefs.isKey("ssid")) keeps working.
static const char *KEY_COUNT   = "wifiCount";

void wifiStoreLoad( WifiCred out[], int &count ) {
    count = 0;
    Preferences prefs;
    prefs.begin( "sys", true );
    int n = prefs.getInt( KEY_COUNT, -1 );
    if ( n > MAX_WIFI_NETWORKS ) {
        n = MAX_WIFI_NETWORKS;
    }

    char k[ 20 ];
    for ( int i = 0; i < n; i++ ) {
        snprintf( k, sizeof( k ), "wifi%d_ssid", i );
        out[ count ].ssid = prefs.getString( k, "" );
        snprintf( k, sizeof( k ), "wifi%d_pass", i );
        out[ count ].pass = deobfuscatePassword( prefs.getString( k, "" ) );
        // Compact: skip entries with an empty SSID (repairs states where
        // wifiCount was written but the entries themselves are missing)
        if ( out[ count ].ssid.length() > 0 ) {
            count++;
        }
    }

    if ( count == 0 ) {
        // No list yet (fresh device) or an inconsistent legacy state —
        // rebuild the list from the single "ssid"/"pass" keys if present
        String legacySsid = prefs.getString( "ssid", "" );
        String legacyPass = deobfuscatePassword( prefs.getString( "pass", "" ) );
        prefs.end();
        if ( legacySsid.length() > 0 ) {
            out[ 0 ].ssid = legacySsid;
            out[ 0 ].pass = legacyPass;
            count = 1;
            wifiStoreSave( out, count );   // writes wifi0_ssid / wifi0_pass too
        }
        return;
    }
    prefs.end();
}

int wifiStoreCount() {
    WifiCred tmp[ MAX_WIFI_NETWORKS ];
    int n = 0;
    wifiStoreLoad( tmp, n );
    return n;
}

void wifiStoreSave( const WifiCred list[], int count ) {
    Preferences prefs;
    prefs.begin( "sys", false );
    prefs.putInt( KEY_COUNT, count );
    char k[ 20 ];
    for ( int i = 0; i < MAX_WIFI_NETWORKS; i++ ) {
        snprintf( k, sizeof( k ), "wifi%d_ssid", i );
        if ( i < count && list[ i ].ssid.length() > 0 ) {
            prefs.putString( k, list[ i ].ssid );
        }
        else {
            prefs.remove( k );
        }
        snprintf( k, sizeof( k ), "wifi%d_pass", i );
        if ( i < count && list[ i ].ssid.length() > 0 ) {
            prefs.putString( k, obfuscatePassword( list[ i ].pass ) );
        }
        else {
            prefs.remove( k );
        }
    }
    prefs.end();
}

void wifiStoreRemember( const String &ssid, const String &pass ) {
    if ( ssid.length() == 0 ) {
        return;
    }
    WifiCred list[ MAX_WIFI_NETWORKS ];
    int n = 0;
    wifiStoreLoad( list, n );

    int found = -1;
    for ( int i = 0; i < n; i++ ) {
        if ( list[ i ].ssid == ssid ) {
            found = i;
            break;
        }
    }

    if ( found >= 0 ) {
        // Existing network — move to front with updated password
        for ( int i = found; i > 0; i-- ) {
            list[ i ] = list[ i - 1 ];
        }
        list[ 0 ].ssid = ssid;
        list[ 0 ].pass = pass;
    }
    else {
        // New network — prepend, drop the oldest when full
        if ( n < MAX_WIFI_NETWORKS ) {
            n++;
        }
        for ( int i = n - 1; i > 0; i-- ) {
            list[ i ] = list[ i - 1 ];
        }
        list[ 0 ].ssid = ssid;
        list[ 0 ].pass = pass;
    }

    wifiStoreSave( list, n );

    // Keep the legacy keys in sync (used by nvsInitialized detection and any
    // code that still reads a single "ssid")
    Preferences prefs;
    prefs.begin( "sys", false );
    prefs.putString( "ssid", ssid );
    prefs.putString( "pass", obfuscatePassword( pass ) );
    prefs.end();
}
