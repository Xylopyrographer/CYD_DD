#include "ota.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <TFT_eSPI.h>

#include "../util/constants.h"
#include "../data/app_state.h"   // ScreenState enum

// Globals owned by main.cpp
extern const char    *FIRMWARE_VERSION;
extern const char    *VERSION_CHECK_URL;
extern String         availableVersion;
extern String         downloadURL;
extern bool           updateAvailable;
extern unsigned long  lastVersionCheck;
extern bool           isUpdating;
extern int            updateProgress;
extern String         updateStatus;
extern ScreenState    currentState;
extern TFT_eSPI       tft;

// UI function defined in main.cpp (Phase 3 will move it to ui/screen_firmware)
void drawFirmwareScreen();

// ============================================================
// isNewerVersion
// ============================================================

bool isNewerVersion( String currentVer, String newVer ) {
    // Strip "v" prefix if present
    currentVer.replace( "v", "" );
    newVer.replace( "v", "" );

    int currMajor = 0, currMinor = 0, currPatch = 0;
    int newMajor = 0, newMinor = 0, newPatch = 0;

    // Parse current version (supports X.Y.Z format)
    int firstDot = currentVer.indexOf( '.' );
    if ( firstDot > 0 ) {
        currMajor = currentVer.substring( 0, firstDot ).toInt();
        int secondDot = currentVer.indexOf( '.', firstDot + 1 );
        if ( secondDot > 0 ) {
            currMinor = currentVer.substring( firstDot + 1, secondDot ).toInt();
            currPatch = currentVer.substring( secondDot + 1 ).toInt();
        }
        else {
            currMinor = currentVer.substring( firstDot + 1 ).toInt();
        }
    }

    // Parse new version (supports X.Y.Z format)
    firstDot = newVer.indexOf( '.' );
    if ( firstDot > 0 ) {
        newMajor = newVer.substring( 0, firstDot ).toInt();
        int secondDot = newVer.indexOf( '.', firstDot + 1 );
        if ( secondDot > 0 ) {
            newMinor = newVer.substring( firstDot + 1, secondDot ).toInt();
            newPatch = newVer.substring( secondDot + 1 ).toInt();
        }
        else {
            newMinor = newVer.substring( firstDot + 1 ).toInt();
        }
    }

    log_d( "[OTA] Comparing versions: %d.%d.%d vs %d.%d.%d", currMajor, currMinor, currPatch, newMajor, newMinor, newPatch );

    if ( newMajor > currMajor ) {
        return true;
    }
    if ( newMajor == currMajor && newMinor > currMinor ) {
        return true;
    }
    if ( newMajor == currMajor && newMinor == currMinor && newPatch > currPatch ) {
        return true;
    }
    return false;
}

// ============================================================
// checkForUpdate
// ============================================================

void checkForUpdate() {
    if ( WiFi.status() != WL_CONNECTED ) {
        log_w( "[OTA] WiFi not connected" );
        return;
    }

    log_i( "[OTA] Checking for updates..." );
    HTTPClient http;

    http.setFollowRedirects( HTTPC_STRICT_FOLLOW_REDIRECTS );
    http.begin( VERSION_CHECK_URL );
    http.setTimeout( HTTP_TIMEOUT_VERSION );

    int httpCode = http.GET();

    if ( httpCode == 200 ) {
        String payload = http.getString();
        log_d( "[OTA] Response: %s", payload.c_str() );

        JsonDocument doc;
        DeserializationError error = deserializeJson( doc, payload );

        if ( !error ) {
            availableVersion = doc[ "version" ].as<String>();
            downloadURL = doc[ "download_url" ].as<String>();

            log_d( "[OTA] Current: %s | Available: %s", FIRMWARE_VERSION, availableVersion.c_str() );

            updateAvailable = isNewerVersion( String( FIRMWARE_VERSION ), availableVersion );

            #ifdef OTA_FORCE_UPDATE
            updateAvailable = true;   // OTA_FORCE_UPDATE: bypass version comparison
            log_w( "[OTA] OTA_FORCE_UPDATE defined — forcing updateAvailable = true" );
            #endif

            if ( updateAvailable ) {
                log_i( "[OTA] New version available!" );
                log_i( "[OTA] Download URL: %s", downloadURL.c_str() );
            }
            else {
                log_i( "[OTA] Already up to date" );
            }
        }
        else {
            log_e( "[OTA] JSON parse error" );
        }
    }
    else {
        log_w( "[OTA] HTTP error: %d", httpCode );
    }

    http.end();
    lastVersionCheck = millis();
}

// ============================================================
// performOTAUpdate
// ============================================================

void performOTAUpdate() {
    if ( !updateAvailable ) {
        log_d( "[OTA] No update available" );
        return;
    }

    isUpdating = true;
    updateProgress = 0;
    updateStatus = "Connecting...";

    tft.fillScreen( TFT_BLACK );
    tft.setTextColor( TFT_WHITE );
    tft.setTextDatum( MC_DATUM );
    tft.drawString( "FIRMWARE UPDATE", 160, 30, 2 );

    if ( downloadURL == "" ) {
        log_w( "[OTA] No download URL available!" );
        tft.fillScreen( TFT_BLACK );
        tft.setTextColor( TFT_RED );
        tft.drawString( "ERROR!", 160, 80, 2 );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "No download URL", 160, 110, 1 );
        delay( 3000 );
        isUpdating = false;
        currentState = FIRMWARE_SETTINGS;
        drawFirmwareScreen();
        return;
    }

    String firmwareURL = downloadURL;
    log_i( "[OTA] Downloading from: %s", firmwareURL.c_str() );
    log_i( "[OTA] Installing version: %s", availableVersion.c_str() );

    HTTPClient http;
    http.setFollowRedirects( HTTPC_STRICT_FOLLOW_REDIRECTS );
    http.begin( firmwareURL );
    http.setTimeout( HTTP_TIMEOUT_OTA );
    int httpCode = http.GET();

    if ( httpCode == 200 ) {
        int contentLength = http.getSize();
        bool canBegin = Update.begin( contentLength );

        if ( canBegin ) {
            WiFiClient *client = http.getStreamPtr();

            size_t written = 0;
            uint8_t buff[ 128 ];
            int lastProgress = -1;

            // ========== PHASE 1: DOWNLOADING ==========
            // Draw static elements once before the loop
            tft.fillRect( 0, 60, 320, 130, TFT_BLACK );
            tft.setTextColor( TFT_CYAN );
            tft.drawString( "Downloading...", 160, 70, 2 );
            tft.drawRoundRect( 40, 100, 240, 25, 4, TFT_DARKGREY );

            while ( http.connected() && ( written < ( size_t )contentLength ) ) {
                size_t available = client->available();
                if ( available ) {
                    size_t bytesRead = client->readBytes( buff, min<size_t>( available, sizeof( buff ) ) );
                    written += Update.write( buff, bytesRead );

                    updateProgress = ( written * 100 ) / contentLength;

                    if ( updateProgress != lastProgress ) {
                        lastProgress = updateProgress;

                        // Extend the fill bar in-place (bar only grows — no clear needed)
                        tft.fillRoundRect( 42, 102, ( updateProgress * 236 ) / 100, 21, 3, TFT_CYAN );
                        // Text with bg fill to erase previous value without a fillRect
                        tft.setTextColor( TFT_WHITE, TFT_BLACK );
                        tft.drawString( String( updateProgress ) + "%", 160, 112, 2 );
                        tft.setTextColor( TFT_LIGHTGREY, TFT_BLACK );
                        String sizeStr = String( written / 1024 ) + " / " + String( contentLength / 1024 ) + " KB";
                        tft.drawString( sizeStr, 160, 140, 1 );

                        if ( updateProgress % 10 == 0 ) {
                            log_d( "[OTA] Downloading: %d%%", updateProgress );
                        }
                    }
                }
                delay( 1 );
            }

            // ========== PHASE 2: INSTALLING ==========
            // Draw static elements once before the loop
            tft.fillRect( 0, 60, 320, 130, TFT_BLACK );
            tft.setTextColor( TFT_ORANGE );
            tft.drawString( "Installing...", 160, 70, 2 );
            tft.drawRoundRect( 40, 100, 240, 25, 4, TFT_DARKGREY );

            for ( int i = 0; i <= 100; i += 5 ) {
                tft.fillRoundRect( 42, 102, ( i * 236 ) / 100, 21, 3, TFT_ORANGE );
                tft.setTextColor( TFT_WHITE, TFT_BLACK );
                tft.drawString( String( i ) + "%", 160, 112, 2 );
                delay( 50 );
            }

            log_d( "[OTA] Finalizing update..." );

            if ( Update.end( true ) ) {
                updateStatus = "Update successful!";
                log_i( "[OTA] Update successful!" );

                tft.fillScreen( TFT_BLACK );
                tft.setTextColor( TFT_GREEN );
                tft.drawString( "UPDATE SUCCESS!", 160, 100, 2 );
                tft.setTextColor( TFT_WHITE );
                tft.drawString( "Rebooting...", 160, 130, 1 );
                delay( 2000 );
                ESP.restart();
            }
            else {
                // ========== FAILURE - ROLLBACK ==========
                updateStatus = "Update failed!";
                log_e( "[OTA] Update failed: %s", Update.errorString() );

                tft.fillScreen( TFT_BLACK );
                tft.setTextColor( TFT_RED );
                tft.drawString( "UPDATE FAILED!", 160, 80, 2 );
                tft.setTextColor( TFT_ORANGE );
                tft.drawString( "Rolling back to", 160, 110, 1 );
                tft.drawString( "previous version...", 160, 125, 1 );

                for ( int i = OTA_COUNTDOWN_SECS; i > 0; i-- ) {
                    tft.fillRect( 140, 150, 40, 20, TFT_BLACK );
                    tft.setTextColor( TFT_WHITE );
                    tft.drawString( String( i ), 160, 160, 2 );
                    delay( 1000 );
                }

                isUpdating = false;
                currentState = FIRMWARE_SETTINGS;
                drawFirmwareScreen();
                return;
            }
        }
        else {
            // ========== INSUFFICIENT SPACE ==========
            updateStatus = "Not enough space!";
            log_e( "[OTA] Not enough space!" );

            tft.fillScreen( TFT_BLACK );
            tft.setTextColor( TFT_RED );
            tft.drawString( "ERROR!", 160, 80, 2 );
            tft.setTextColor( TFT_WHITE );
            tft.drawString( "Not enough storage", 160, 110, 1 );
            tft.drawString( "space for update", 160, 125, 1 );

            for ( int i = OTA_COUNTDOWN_SECS; i > 0; i-- ) {
                tft.fillRect( 140, 150, 40, 20, TFT_BLACK );
                tft.drawString( String( i ), 160, 160, 2 );
                delay( 1000 );
            }

            isUpdating = false;
            currentState = FIRMWARE_SETTINGS;
            drawFirmwareScreen();
            http.end();
            return;
        }
    }
    else {
        // ========== DOWNLOAD ERROR ==========
        updateStatus = "Download failed!";
        log_w( "[OTA] HTTP error: %d", httpCode );

        tft.fillScreen( TFT_BLACK );
        tft.setTextColor( TFT_RED );
        tft.drawString( "DOWNLOAD FAILED!", 160, 80, 2 );
        tft.setTextColor( TFT_WHITE );
        tft.drawString( "HTTP Error: " + String( httpCode ), 160, 110, 1 );
        tft.drawString( "Check your connection", 160, 125, 1 );

        for ( int i = OTA_COUNTDOWN_SECS; i > 0; i-- ) {
            tft.fillRect( 140, 150, 40, 20, TFT_BLACK );
            tft.drawString( String( i ), 160, 160, 2 );
            delay( 1000 );
        }

        isUpdating = false;
        currentState = FIRMWARE_SETTINGS;
        drawFirmwareScreen();
        http.end();
        return;
    }

    http.end();
    isUpdating = false;
}
