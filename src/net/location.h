#pragma once
#include <Arduino.h>

bool lookupCountryRESTAPI( String countryName );
bool lookupCountryEmbedded( String countryName );
bool lookupCountryGeonames( String countryName );
bool lookupCityNominatim( String cityName, String countryHint );
bool lookupCityGeonames( String cityName, String countryHint );
