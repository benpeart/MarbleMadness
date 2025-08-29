#include "main.h"
#include "debug.h"
#ifdef WIFI
#include <WiFi.h>
#include "WiFiHelpers.h"
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "settings.h"
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#ifdef TIME
#include "RealTimeClock.h"
#endif // TIME

#ifdef DRD
// https://github.com/khoih-prog/ESP_DoubleResetDetector
#define ESP_DRD_USE_EEPROM true

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

#include <ESP_DoubleResetDetector.h>

DoubleResetDetector *drd;
#endif // DRD

#define MAX_HOSTNAME_LEN 32
char hostname[MAX_HOSTNAME_LEN] = "MarbleMadness";

// Indicates whether ESP has WiFi credentials saved from previous session, or double reset detected
bool initialConfig = false;

#define HTTP_PORT 80
AsyncWebServer webServer(HTTP_PORT);
DNSServer dnsServer;

void wifi_setup(void)
{
    // connect to wifi or enter AP mode so it can be configured
    preferences.getBytes("hostname", hostname, sizeof(hostname));
    hostname[MAX_HOSTNAME_LEN - 1] = 0; // ensure it is null terminated
    WiFi.setHostname(hostname);

    // connect to wifi or enter AP mode so it can be configured
#ifdef DRD
    drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
    if (drd->detectDoubleReset())
    {
        DB_PRINTLN("Double reset detected");
        initialConfig = true;
    }
#endif

    // Local intialization. Once its business is done, there is no need to keep it around
    AsyncWiFiManager wifiManager(&webServer, &dnsServer);

    if (initialConfig)
    {
        DB_PRINTLN(F("Starting Config Portal"));

        // initial config, disable timeout.
        wifiManager.setConfigPortalTimeout(0);

        wifiManager.startConfigPortal((String(hostname) + "AP").c_str());
    }
    else
    {
        // Give 2 minutes to configure WiFi, otherwise, just go into MarbleMadness mode without it
        wifiManager.setConfigPortalTimeout(120);

        wifiManager.autoConnect((String(hostname) + "AP").c_str());
    }

    // report on our WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        DB_PRINT(F("Connected. Local IP: "));
        DB_PRINTLN(WiFi.localIP());
    }

    // Setup for OTA updates
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA
        .onStart([]()
                 {
                    String type;
                    if (ArduinoOTA.getCommand() == U_FLASH)
                    {
                        type = "sketch";
                    } else { // U_SPIFFS
                        type = "filesystem";
                        // TODO: is this needed?
                        // SPIFFS.end();
                    }
                    DB_PRINTLN("Start updating " + type); })
        .onEnd([]()
               { DB_PRINTLN("\nEnd"); })
        .onProgress([](unsigned int progress, unsigned int total)
                    { DB_PRINTF("Progress: %u%%\r", (progress / (total / 100))); })
        .onError([](ota_error_t error)
                 {
                    DB_PRINTF("Error[%u]: ", error);
                    if (error == OTA_AUTH_ERROR) DB_PRINTLN("Auth Failed");
                    else if (error == OTA_BEGIN_ERROR) DB_PRINTLN("Begin Failed");
                    else if (error == OTA_CONNECT_ERROR) DB_PRINTLN("Connect Failed");
                    else if (error == OTA_RECEIVE_ERROR) DB_PRINTLN("Receive Failed");
                    else if (error == OTA_END_ERROR) DB_PRINTLN("End Failed"); });

    ArduinoOTA.begin();
    DB_PRINTLN("Ready for OTA updates");

    // Start MDNS server
    if (MDNS.begin(hostname))
    {
        // Advertise HTTP service on port 80
        MDNS.addService("http", "tcp", 80);
        DB_PRINT("MDNS responder started, name: ");
        DB_PRINTLN(hostname);
    }
    else
    {
        DB_PRINTLN("Could not start MDNS responder");
    }

    // Setup the web UI by serveing static files from LittleFS
    if (!LittleFS.begin(false))
    {
        DB_PRINTLN("LittleFS mount failed");
    }
    else
    {
        webServer.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
        DB_PRINTLN("LittleFS mount success");
    }
    webServer.onNotFound([](AsyncWebServerRequest *request)
                         { request->send(404, "text/plain", "FileNotFound"); });
#ifdef SPIFFSEDITOR
    httpServer->addHandler(new SPIFFSEditor(SPIFFS));
#endif // SPIFFSEDITOR

    webServer.begin();

#ifdef TIME
    // intialize the real time clock
    rtc_setup();
#endif
}

void wifi_loop(void)
{
#ifdef DRD
    // Call the double reset detector loop method every so often so that it can recognize when the timeout expires.
    // You can also call drd.stop() when you wish to no longer consider the next reset as a double reset.
    drd->loop();
#endif

    // check for OTA updates
    ArduinoOTA.handle();

    // if we lost WiFi, try to reconnect
    if ((WiFi.status() != WL_CONNECTED))
    {
        DB_PRINTLN(F("\nWiFi lost. Attempting to reconnect"));

        // Local intialization. Once its business is done, there is no need to keep it around
        AsyncWiFiManager wifiManager(&webServer, &dnsServer);

        // attempt to reconnect
        wifiManager.autoConnect((String(hostname) + "AP").c_str());

        // report on our WiFi connection status
        if (WiFi.status() == WL_CONNECTED)
        {
            DB_PRINT(F("Connected. Local IP: "));
            DB_PRINTLN(WiFi.localIP());
        }
    }
}
#endif // WIFI
