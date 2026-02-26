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

String removeDiacritics( String input ) {
    String output = input;
    // Lowercase letters
    output.replace( "á", "a" );
    output.replace( "č", "c" );
    output.replace( "ď", "d" );
    output.replace( "é", "e" );
    output.replace( "ě", "e" );
    output.replace( "í", "i" );
    output.replace( "ľ", "l" );
    output.replace( "ĺ", "l" );
    output.replace( "ň", "n" );
    output.replace( "ó", "o" );
    output.replace( "ô", "o" );
    output.replace( "ř", "r" );
    output.replace( "š", "s" );
    output.replace( "ť", "t" );
    output.replace( "ú", "u" );
    output.replace( "ů", "u" );
    output.replace( "ý", "y" );
    output.replace( "ž", "z" );

    // Uppercase letters
    output.replace( "Á", "A" );
    output.replace( "Č", "C" );
    output.replace( "Ď", "D" );
    output.replace( "É", "E" );
    output.replace( "Ě", "E" );
    output.replace( "Í", "I" );
    output.replace( "Ľ", "L" );
    output.replace( "Ĺ", "L" );
    output.replace( "Ň", "N" );
    output.replace( "Ó", "O" );
    output.replace( "Ô", "O" );
    output.replace( "Ř", "R" );
    output.replace( "Š", "S" );
    output.replace( "Ť", "T" );
    output.replace( "Ú", "U" );
    output.replace( "Ů", "U" );
    output.replace( "Ý", "Y" );
    output.replace( "Ž", "Z" );

    return output;
}
