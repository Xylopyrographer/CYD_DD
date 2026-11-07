#pragma once

#include <Arduino.h>

// Multi-network WiFi memory — behaves like a phone: every network the user
// configures is remembered (up to MAX_WIFI_NETWORKS, most-recent-first), and
// at boot the device connects to whichever one is reachable.
constexpr int MAX_WIFI_NETWORKS = 4;

struct WifiCred {
    String ssid;
    String pass;   // plain text in RAM (obfuscated only in NVS)
};

// Load all saved networks into out[] (most-recent-first). On first run it
// migrates the legacy single "ssid"/"pass" keys into the list automatically.
void wifiStoreLoad( WifiCred out[], int &count );

// How many networks are currently remembered.
int wifiStoreCount();

// Persist the list (passwords are obfuscated on the way in).
void wifiStoreSave( const WifiCred list[], int count );

// Add or update a network: same SSID updates its password and moves to the
// front; unknown SSIDs are prepended and the oldest entry is dropped when the
// list is full. Also refreshes the legacy "ssid"/"pass" keys used elsewhere.
void wifiStoreRemember( const String &ssid, const String &pass );
