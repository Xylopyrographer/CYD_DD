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
        String city = prefs.getString( ( prefix + "c" ).c_str(), "" );
        if ( city.length() == 0 ) {
            break;
        }
        recentCities[ i ].city = city;
        recentCities[ i ].country = prefs.getString( ( prefix + "co" ).c_str(), "" );
        recentCities[ i ].timezone = prefs.getString( ( prefix + "tz" ).c_str(), "" );
        recentCities[ i ].gmtOffset = prefs.getInt( ( prefix + "go" ).c_str(), 3600 );
        recentCities[ i ].dstOffset = prefs.getInt( ( prefix + "do" ).c_str(), 3600 );
        recentCount++;
    }
    prefs.end();
}

void addToRecentCities( String city, String country, String timezone, int gmtOffset, int dstOffset ) {
    for ( int i = 0; i < recentCount; i++ ) {
        if ( recentCities[ i ].city == city && recentCities[ i ].country == country ) {
            RecentCity temp = recentCities[ i ];
            for ( int j = i; j > 0; j-- ) {
                recentCities[ j ] = recentCities[ j - 1 ];
            }
            recentCities[ 0 ] = temp;
            return;
        }
    }
    if ( recentCount < MAX_RECENT_CITIES ) {
        for ( int i = recentCount - 1; i >= 0; i-- ) {
            recentCities[ i + 1 ] = recentCities[ i ];
        }
    }
    else {
        recentCount = MAX_RECENT_CITIES - 1;
        for ( int i = recentCount - 1; i >= 0; i-- ) {
            recentCities[ i + 1 ] = recentCities[ i ];
        }
    }
    recentCities[ 0 ].city = city;
    recentCities[ 0 ].country = country;
    recentCities[ 0 ].timezone = timezone;
    recentCities[ 0 ].gmtOffset = gmtOffset;
    recentCities[ 0 ].dstOffset = dstOffset;

    prefs.begin( "sys", false );
    for ( int i = 0; i < recentCount; i++ ) {
        String prefix = "recent" + String( i );
        prefs.putString( ( prefix + "c" ).c_str(), recentCities[ i ].city );
        prefs.putString( ( prefix + "co" ).c_str(), recentCities[ i ].country );
        prefs.putString( ( prefix + "tz" ).c_str(), recentCities[ i ].timezone );
        prefs.putInt( ( prefix + "go" ).c_str(), recentCities[ i ].gmtOffset );
        prefs.putInt( ( prefix + "do" ).c_str(), recentCities[ i ].dstOffset );
    }
    prefs.end();
}

