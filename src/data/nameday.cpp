#include "nameday.h"
#include "time.h"

// ── Globals defined here ───────────────────────────────────────────────────────
String todayNameday   = "--";
int    lastNamedayDay  = -1;
int    lastNamedayHour = -1;
bool   namedayValid    = false;

// ── External globals owned by main.cpp ────────────────────────────────────────
extern String selectedCountry;
extern bool   forceClockRedraw;

// ── Czech nameday data table ───────────────────────────────────────────────────
// Index: [month 1-12][day 1-31].  Row 0 and column 0 are unused sentinels.
// Names are stored without diacritics for maximum display compatibility.
static const char *namedays[ 13 ][ 32 ] = {
    {}, // month 0 — unused
    {"--", "Novy rok",       "Karina",   "Radmila",  "Diana",     "Dalimil",     "Tri krále",     "Vilma",    "Ctirad",   "Adrian",    "Brezislav",   "Bohdana",  "Pravoslav", "Edita",      "Radovan",    "Alice",      "Ctirad",    "Drahoslav",  "Vladislav",  "Doubravka", "Ilona",   "Elian",    "Slavomir", "Zdenek",   "Milena",  "Milos",  "Zora",    "Ingrid",  "Otyla",    "Zdislava", "Robin",     "Marika"},      // January
    {"--", "Hynek",          "Nela",     "Blazej",   "Jarmila",   "Dobromila",   "Vanda",         "Veronika", "Milada",   "Apolena",   "Mojmir",      "Bozena",   "Slavena",   "Vendelin",   "Valentin",   "Jiri",       "Ljuba",     "Miloslav",   "Gizela",     "Patrik",    "Oldrich", "Lenka",    "Petr",     "Svatopluk", "Matej",   "Liliana", "Dorotea", "Alexandr", "Lumír",    "Horymír",  "--",        "--"},       // February
    {"--", "Bedrich",        "Anezka",   "Kamil",    "Stela",     "Kazimir",     "Miroslav",      "Tomas",    "Gabriela", "Franciska", "Viktorie",    "Andelka",  "Rehore",    "Ruzena",     "Matylda",    "Kristyna",   "Lubomir",   "Vlastimil",  "Eduard",     "Josef",     "Svetlana", "Radek",    "Leona",    "Ivona",    "Gabriel", "Marian", "Emanuel", "Dita",    "Sonar",    "Taťana",   "Arnošt",    "Kveta"},      // March
    {"--", "Hugo",           "Erika",    "Richard",  "Ivana",     "Miroslava",   "Vendula",       "Herman",   "Ema",      "Dusan",     "Darja",       "Izabela",  "Julius",    "Ales",       "Vincenc",    "Anastázie",  "Irena",     "Rudolf",     "Valerie",    "Rostislav", "Marcela", "Alexandr", "Evženie",  "Vojtech",  "Jiri",    "Marek",  "Oto",     "Jaroslav", "Vlastislav", "Robert",   "Blahoslav", "--"},        // April
    {"--", "Svátek práce",   "Zikmund",  "Alexej",   "Květoslav", "Klaudie",     "Radoslav",      "Stanislav", "Den vítězství", "Ctibor", "Blažena",    "Svatava",  "Pankrac",   "Servác",     "Bonifác",    "Žofie",      "Přemysl",   "Aneta",      "Nataša",     "Ivo",       "Zbyšek",  "Monika",   "Emil",     "Vladimír", "Jana",    "Viola",  "Filip",   "Valdemar", "Vilém",    "Maxim",    "Ferdinand", "Kamila"}, // May
    {"--", "Laura",          "Jarmil",   "Tamara",   "Dalibor",   "Dobroslav",   "Norbert",       "Iveta",    "Medard",   "Stanislava", "Gita",        "Bruno",    "Antonie",   "Antonín",    "Roland",     "Vít",        "Zbyněk",    "Adolf",      "Milan",      "Leoš",      "Květa",   "Alois",    "Pavla",    "Zdeňka",   "Jan",     "Ivan",   "Adriana", "Ladislav", "Lubomír",  "Petr a Pavel", "Šárka",  "--"},     // June
    {"--", "Jaroslava",      "Patricie", "Radomír",  "Prokop",    "Cyril a Metoděj", "Jan Hus",    "Bohuslava", "Nora",     "Drahoslava", "Libuše a Amálie", "Olga",  "Bořek",     "Markéta",    "Karolína",   "Jindřich",   "Luboš",     "Martina",    "Drahomíra",  "Čeněk",     "Ilja",    "Vítězslav", "Magdaléna", "Libor",    "Kristýna", "Jakub",  "Anna",    "Věroslav", "Viktor",   "Marta",    "Bořivoj",   "Ignác"}, // July
    {"--", "Oskar",          "Gustav",   "Miluše",   "Dominik",   "Kristián",    "Oldřiška",      "Lada",     "Soběslav", "Roman",     "Vavřinec",    "Zuzana",   "Klára",     "Alena",      "Alan",       "Hana",       "Jáchym",    "Petra",      "Helena",     "Ludvík",    "Bernard", "Johana",   "Bohuslav", "Sandra",   "Bartoloměj", "Radim", "Luděk",   "Otakar",  "Augustýn", "Evelína",  "Vladěna",   "Pavlína"}, // August
    {"--", "Linda",          "Adéla",    "Bronislav", "Jindřiška", "Boris",       "Boleslav",      "Regína",   "Mariana",  "Daniela",   "Irma",        "Denisa",   "Marie",     "Lubor",      "Radka",      "Jolana",     "Ludmila",   "Naděžda",    "Kryštof",    "Zita",      "Oleg",    "Matouš",   "Darina",   "Berta",    "Jaromír", "Zlata",  "Andrea",  "Jonáš",   "Václav",   "Michal",   "Jeroným",   "--"},        // September
    {"--", "Igor",           "Olivie",   "Bohumil",  "František", "Eliška",      "Hanuš",         "Justýna",  "Věra",     "Štefan",    "Marina",      "Andrej",   "Marcel",    "Renáta",     "Agáta",      "Tereza",     "Havel",     "Hedvika",    "Lukáš",      "Michaela",  "Vendelín", "Brigita",  "Sabina",   "Teodor",   "Nina",    "Beáta",  "Erik",    "Šarlota", "Státní svátek", "Silvie", "Tadeáš",   "Štěpánka"}, // October
    {"--", "Felix",          "Památka zesnulých", "Hubert", "Karel", "Miriam",     "Liběna",        "Saskie",   "Bohumír",  "Bohdan",    "Evžen",       "Martin",   "Benedikt",  "Tibor",      "Sáva",       "Leopold",    "Otmar",     "Den boje za svobodu", "Romana", "Alžběta", "Nikola",  "Albert",   "Cecílie",  "Klement",  "Emílie",  "Kateřina", "Artur",   "Xenie",   "René",     "Zina",     "Ondřej",    "--"},    // November
    {"--", "Iva",            "Blanka",   "Svatoslav", "Barbora",   "Jitka",       "Mikuláš",       "Ambrož",   "Květoslava", "Vratislav", "Julie",       "Dana",     "Simona",    "Lucie",      "Lýdie",      "Radana",     "Albína",    "Daniel",     "Miloslav",   "Ester",     "Dagmar",  "Natálie",  "Šimon",    "Vlasta",   "Štědrý den", "1. svátek vánoční", "2. svátek vánoční", "Žaneta", "Bohumila", "Judita", "David", "Silvestr"} // December
};

// ── Public functions ───────────────────────────────────────────────────────────

String getNamedayForDate( int day, int month ) {
    if ( month < 1 || month > 12 || day < 1 || day > 31 ) {
        return "--";
    }
    return String( namedays[ month ][ day ] );
}

void handleNamedayUpdate() {
    // Only for Czech Republic — hardcoded Czech name table
    if ( selectedCountry != "Czech Republic" ) {
        namedayValid  = false;
        todayNameday  = "--";
        return;
    }

    time_t    now      = time( nullptr );
    struct tm *timeinfo = localtime( &now );

    if ( !timeinfo ) {
        namedayValid = false;
        todayNameday = "--";
        return;
    }

    // Ignore invalid / un-synced time (year < 2025)
    if ( timeinfo->tm_year < 125 ) {
        namedayValid = false;
        todayNameday = "--";
        return;
    }

    int today = timeinfo->tm_mday;
    int month = timeinfo->tm_mon + 1;
    int hour  = timeinfo->tm_hour;

    // Update when the day changes
    if ( today != lastNamedayDay ) {
        lastNamedayDay  = today;
        lastNamedayHour = hour;

        log_d( "[NAMEDAY] Getting nameday for day %d.%d", today, month );

        todayNameday = getNamedayForDate( today, month );
        namedayValid = ( todayNameday != "--" );

        if ( namedayValid ) {
            log_d( "[NAMEDAY] SUCCESS: %s", todayNameday.c_str() );
            forceClockRedraw = true;
        }
        else {
            log_d( "[NAMEDAY] No nameday for this date" );
        }
    }
    else if ( hour == 0 && lastNamedayHour != 0 ) {
        // Midnight crossover check
        lastNamedayHour = hour;

        time_t now2 = time( nullptr );
        struct tm *timeinfo2 = localtime( &now2 );
        if ( timeinfo2 && timeinfo2->tm_mday != lastNamedayDay ) {
            lastNamedayDay = timeinfo2->tm_mday;
            month          = timeinfo2->tm_mon + 1;
            todayNameday   = getNamedayForDate( lastNamedayDay, month );
            namedayValid   = ( todayNameday != "--" );

            if ( namedayValid ) {
                log_d( "[NAMEDAY] Midnight update: %s", todayNameday.c_str() );
                forceClockRedraw = true;
            }
        }
    }
    else {
        lastNamedayHour = hour;
    }
}
