#include "string_utils.h"

String toTitleCase( String input ) {
    input.toLowerCase();
    if ( input.length() > 0 ) {
        input[ 0 ] = toupper( input[ 0 ] );
    }
    for ( int i = 1; i < input.length(); i++ ) {
        if ( input[ i - 1 ] == ' ' ) {
            input[ i ] = toupper( input[ i ] );
        }
    }
    return input;
}

bool fuzzyMatch( String input, String target ) {
    String inp = input;
    String tgt = target;
    inp.toLowerCase();
    tgt.toLowerCase();
    if ( inp == tgt ) {
        return true;
    }
    if ( tgt.indexOf( inp ) >= 0 ) {
        return true;
    }
    if ( inp.length() >= 3 && tgt.startsWith( inp ) ) {
        return true;
    }
    return false;
}


