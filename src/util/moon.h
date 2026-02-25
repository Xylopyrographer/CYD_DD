#pragma once

// Returns the moon phase (0–7) for the given date.
// 0=New, 1=Waxing Crescent, 2=First Quarter, 3=Waxing Gibbous,
// 4=Full, 5=Waning Gibbous, 6=Last Quarter, 7=Waning Crescent
int getMoonPhase( int year, int month, int day );
