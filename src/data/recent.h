#pragma once
#include <Arduino.h>
#include "app_state.h"

void loadRecentCities();
void addToRecentCities( String city, String country, String timezone,
                         int gmtOffset, int dstOffset );
