#include "recent.h"
#include <Preferences.h>
#include "../util/constants.h"

// Externs defined in main.cpp
extern RecentCity recentCities[];
extern int        recentCount;
extern Preferences prefs;

void loadRecentCities() {
    prefs.begin( "sys", false );
    for ( int i = 0; i < MAX_RECENT_CITIES; i++ ) {
        String prefix = "recent" + String( i );
        if ( !prefs.isKey( ( prefix + "c" ).c_str() ) ) {
            break;
        }
        recentCities[ i ].city    = prefs.getString( ( prefix + "c" ).c_str(), "" );
        recentCities[ i ].country  = prefs.getString( ( prefix + "co" ).c_str(), "" );
        recentCities[ i ].timezone = prefs.getString( ( prefix + "tz" ).c_str(), "" );
        recentCities[ i ].gmtOffset = prefs.getInt( ( prefix + "go" ).c_str(), 3600 );
        recentCities[ i ].dstOffset = prefs.getInt( ( prefix + "do" ).c_str(), 3600 );
        recentCount++;
    }
    prefs.end();
}

