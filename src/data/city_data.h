#pragma once

// ================= CITY / COUNTRY DATA =================
// Static lookup tables for the regional settings screen.
// Each CityEntry holds the city name, IANA timezone string, and UTC/DST offsets.
// CountryEntry groups a set of cities under a two-letter ISO country code.

struct CityEntry {
    const char *name;
    const char *timezone;
    int gmtOffset;
    int dstOffset;
};

// ── Individual country city lists ────────────────────────────────────────────

const CityEntry czechCities[] = {
    {"Brno",              "Europe/Prague", 3600, 3600},
    {"Ceska Budejovice",  "Europe/Prague", 3600, 3600},
    {"Jihlava",           "Europe/Prague", 3600, 3600},
    {"Karlovy Vary",      "Europe/Prague", 3600, 3600},
    {"Liberec",           "Europe/Prague", 3600, 3600},
    {"Olomouc",           "Europe/Prague", 3600, 3600},
    {"Ostrava",           "Europe/Prague", 3600, 3600},
    {"Pardubice",         "Europe/Prague", 3600, 3600},
    {"Plzen",             "Europe/Prague", 3600, 3600},
    {"Praha",             "Europe/Prague", 3600, 3600},
};

const CityEntry slovakCities[] = {
    {"Banska Bystrica",  "Europe/Bratislava", 3600, 3600},
    {"Bardejov",         "Europe/Bratislava", 3600, 3600},
    {"Bratislava",       "Europe/Bratislava", 3600, 3600},
    {"Kosice",           "Europe/Bratislava", 3600, 3600},
    {"Liptovsky Mikulas", "Europe/Bratislava", 3600, 3600},
    {"Lucenec",          "Europe/Bratislava", 3600, 3600},
    {"Nitra",            "Europe/Bratislava", 3600, 3600},
    {"Poprad",           "Europe/Bratislava", 3600, 3600},
    {"Presov",           "Europe/Bratislava", 3600, 3600},
    {"Zilina",           "Europe/Bratislava", 3600, 3600},
};

const CityEntry germanyCities[] = {
    {"Aachen",    "Europe/Berlin", 3600, 3600},
    {"Berlin",    "Europe/Berlin", 3600, 3600},
    {"Cologne",   "Europe/Berlin", 3600, 3600},
    {"Dortmund",  "Europe/Berlin", 3600, 3600},
    {"Dresden",   "Europe/Berlin", 3600, 3600},
    {"Dusseldorf", "Europe/Berlin", 3600, 3600},
    {"Essen",     "Europe/Berlin", 3600, 3600},
    {"Frankfurt", "Europe/Berlin", 3600, 3600},
    {"Hamburg",   "Europe/Berlin", 3600, 3600},
    {"Munich",    "Europe/Berlin", 3600, 3600},
};

const CityEntry austriaCities[] = {
    {"Dornbirn",     "Europe/Vienna", 3600, 3600},
    {"Graz",         "Europe/Vienna", 3600, 3600},
    {"Hallein",      "Europe/Vienna", 3600, 3600},
    {"Innsbruck",    "Europe/Vienna", 3600, 3600},
    {"Klagenfurt",   "Europe/Vienna", 3600, 3600},
    {"Linz",         "Europe/Vienna", 3600, 3600},
    {"Salzburg",     "Europe/Vienna", 3600, 3600},
    {"Sankt Polten", "Europe/Vienna", 3600, 3600},
    {"Vienna",       "Europe/Vienna", 3600, 3600},
    {"Wels",         "Europe/Vienna", 3600, 3600},
};

const CityEntry polonyCities[] = {
    {"Bialystok", "Europe/Warsaw", 3600, 3600},
    {"Bydgoszcz", "Europe/Warsaw", 3600, 3600},
    {"Cracow",    "Europe/Warsaw", 3600, 3600},
    {"Gdansk",    "Europe/Warsaw", 3600, 3600},
    {"Gdynia",    "Europe/Warsaw", 3600, 3600},
    {"Katowice",  "Europe/Warsaw", 3600, 3600},
    {"Krakow",    "Europe/Warsaw", 3600, 3600},
    {"Poznan",    "Europe/Warsaw", 3600, 3600},
    {"Szczecin",  "Europe/Warsaw", 3600, 3600},
    {"Warsaw",    "Europe/Warsaw", 3600, 3600},
};

const CityEntry franceCities[] = {
    {"Amiens",    "Europe/Paris", 3600, 3600},
    {"Bordeaux",  "Europe/Paris", 3600, 3600},
    {"Brest",     "Europe/Paris", 3600, 3600},
    {"Dijon",     "Europe/Paris", 3600, 3600},
    {"Grenoble",  "Europe/Paris", 3600, 3600},
    {"Lille",     "Europe/Paris", 3600, 3600},
    {"Lyon",      "Europe/Paris", 3600, 3600},
    {"Marseille", "Europe/Paris", 3600, 3600},
    {"Paris",     "Europe/Paris", 3600, 3600},
    {"Toulouse",  "Europe/Paris", 3600, 3600},
};

const CityEntry unitedStatesCities[] = {
    {"Atlanta",       "America/New_York",     -18000, 3600},
    {"Boston",        "America/New_York",     -18000, 3600},
    {"Charlotte",     "America/New_York",     -18000, 3600},
    {"Chicago",       "America/Chicago",      -21600, 3600},
    {"Dallas",        "America/Chicago",      -21600, 3600},
    {"Denver",        "America/Denver",       -25200, 3600},
    {"Detroit",       "America/New_York",     -18000, 3600},
    {"Houston",       "America/Chicago",      -21600, 3600},
    {"Los Angeles",   "America/Los_Angeles",  -28800, 3600},
    {"Miami",         "America/New_York",     -18000, 3600},
    {"New York",      "America/New_York",     -18000, 3600},
    {"Philadelphia",  "America/New_York",     -18000, 3600},
    {"Phoenix",       "America/Phoenix",      -25200, 0   },
    {"San Francisco", "America/Los_Angeles",  -28800, 3600},
    {"Seattle",       "America/Los_Angeles",  -28800, 3600},
};

const CityEntry unitedKingdomCities[] = {
    {"Bath",        "Europe/London", 0, 3600},
    {"Belfast",     "Europe/London", 0, 3600},
    {"Birmingham",  "Europe/London", 0, 3600},
    {"Bristol",     "Europe/London", 0, 3600},
    {"Cardiff",     "Europe/London", 0, 3600},
    {"Edinburgh",   "Europe/London", 0, 3600},
    {"Leeds",       "Europe/London", 0, 3600},
    {"Liverpool",   "Europe/London", 0, 3600},
    {"London",      "Europe/London", 0, 3600},
    {"Manchester",  "Europe/London", 0, 3600},
};

const CityEntry japanCities[] = {
    {"Aomori",    "Asia/Tokyo", 32400, 0},
    {"Fukuoka",   "Asia/Tokyo", 32400, 0},
    {"Hiroshima", "Asia/Tokyo", 32400, 0},
    {"Kobe",      "Asia/Tokyo", 32400, 0},
    {"Kyoto",     "Asia/Tokyo", 32400, 0},
    {"Nagoya",    "Asia/Tokyo", 32400, 0},
    {"Osaka",     "Asia/Tokyo", 32400, 0},
    {"Sapporo",   "Asia/Tokyo", 32400, 0},
    {"Tokyo",     "Asia/Tokyo", 32400, 0},
    {"Yokohama",  "Asia/Tokyo", 32400, 0},
};

const CityEntry australiaCities[] = {
    {"Adelaide",   "Australia/Adelaide",  34200, 3600},
    {"Brisbane",   "Australia/Brisbane",  36000, 0   },
    {"Canberra",   "Australia/Sydney",    36000, 3600},
    {"Darwin",     "Australia/Darwin",    34200, 0   },
    {"Hobart",     "Australia/Hobart",    36000, 3600},
    {"Melbourne",  "Australia/Melbourne", 36000, 3600},
    {"Perth",      "Australia/Perth",     28800, 0   },
    {"Sydney",     "Australia/Sydney",    36000, 3600},
    {"Townsville", "Australia/Brisbane",  36000, 0   },
    {"Wollongong", "Australia/Sydney",    36000, 3600},
};

const CityEntry chinaCities[] = {
    {"Beijing",   "Asia/Shanghai",   28800, 0},
    {"Chongqing", "Asia/Shanghai",   28800, 0},
    {"Guangzhou", "Asia/Shanghai",   28800, 0},
    {"Hangzhou",  "Asia/Shanghai",   28800, 0},
    {"Hong Kong", "Asia/Hong_Kong",  28800, 0},
    {"Shanghai",  "Asia/Shanghai",   28800, 0},
    {"Shenzhen",  "Asia/Shanghai",   28800, 0},
    {"Tianjin",   "Asia/Shanghai",   28800, 0},
    {"Wuhan",     "Asia/Shanghai",   28800, 0},
    {"Xian",      "Asia/Shanghai",   28800, 0},
};

// ── Country directory ─────────────────────────────────────────────────────────

struct CountryEntry {
    const char     *code;
    const char     *name;
    const CityEntry *cities;
    int             cityCount;
};

const CountryEntry countries[] = {
    {"AT", "Austria",        austriaCities,        10},
    {"AU", "Australia",      australiaCities,      10},
    {"CN", "China",          chinaCities,          10},
    {"CZ", "Czech Republic", czechCities,          10},
    {"DE", "Germany",        germanyCities,        10},
    {"FR", "France",         franceCities,         10},
    {"GB", "United Kingdom", unitedKingdomCities,  10},
    {"JP", "Japan",          japanCities,          10},
    {"PL", "Poland",         polonyCities,         10},
    {"SK", "Slovakia",       slovakCities,         10},
    {"US", "United States",  unitedStatesCities,   15},
};

const int COUNTRIES_COUNT = 11;


// ─── Inline lookup helpers ─────────────────────────────────────────────────────

inline void getCountryCities( String countryName, String cities[], int &count ) {
    count = 0;
    for ( int i = 0; i < COUNTRIES_COUNT; i++ ) {
        if ( String( countries[ i ].name ) == countryName ) {
            count = countries[ i ].cityCount;
            for ( int j = 0; j < count; j++ ) {
                cities[ j ] = countries[ i ].cities[ j ].name;
            }
            return;
        }
    }
}

inline bool getTimezoneForCity( String countryName, String city,
                                 String &timezone, int &gmt, int &dst ) {
    for ( int i = 0; i < COUNTRIES_COUNT; i++ ) {
        if ( String( countries[ i ].name ) == countryName ) {
            for ( int j = 0; j < countries[ i ].cityCount; j++ ) {
                if ( String( countries[ i ].cities[ j ].name ) == city ) {
                    timezone = countries[ i ].cities[ j ].timezone;
                    gmt      = countries[ i ].cities[ j ].gmtOffset;
                    dst      = countries[ i ].cities[ j ].dstOffset;
                    return true;
                }
            }
        }
    }
    return false;
}
