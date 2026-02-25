#include "moon.h"

#include <math.h>

int getMoonPhase( int y, int m, int d ) {
    // More accurate moon phase calculation
    // Based on an astronomical algorithm with accuracy to days

    // Calculate Julian Date Number
    if ( m < 3 ) {
        y--;
        m += 12;
    }

    int a = y / 100;
    int b = 2 - a + ( a / 4 );

    long jd = ( long )( 365.25 * ( y + 4716 ) ) +
              ( long )( 30.6001 * ( m + 1 ) ) +
              d + b - 1524;

    // Calculate moon phase
    // Reference new moon: 6 January 2000, 18:14 UTC (JD 2451550.26)
    double daysSinceNew = jd - 2451550.1;

    // Lunar cycle is 29.53058867 days
    double lunationCycle = 29.53058867;
    double currentLunation = daysSinceNew / lunationCycle;

    // Position in current cycle (0.0 - 1.0)
    double phasePosition = currentLunation - floor( currentLunation );

    // Convert to 8 phases (0-7) with accurate boundaries
    // Each phase spans 1/8 of the cycle, boundaries are at midpoints
    int phase;
    if ( phasePosition < 0.0625 ) {
        phase = 0;    // New Moon (0.000 - 0.062)
    }
    else if ( phasePosition < 0.1875 ) {
        phase = 1;    // Waxing Crescent (0.062 - 0.188)
    }
    else if ( phasePosition < 0.3125 ) {
        phase = 2;    // First Quarter (0.188 - 0.312)
    }
    else if ( phasePosition < 0.4375 ) {
        phase = 3;    // Waxing Gibbous (0.312 - 0.438)
    }
    else if ( phasePosition < 0.5625 ) {
        phase = 4;    // Full Moon (0.438 - 0.562)
    }
    else if ( phasePosition < 0.6875 ) {
        phase = 5;    // Waning Gibbous (0.562 - 0.688)
    }
    else if ( phasePosition < 0.8125 ) {
        phase = 6;    // Last Quarter (0.688 - 0.812)
    }
    else if ( phasePosition < 0.9375 ) {
        phase = 7;    // Waning Crescent (0.812 - 0.938)
    }
    else {
        phase = 0;    // New Moon (0.938 - 1.000)
    }

    return phase;
}
