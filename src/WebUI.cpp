#include "main.h"
#include "debug.h"
#include "WebUI.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

void WebUI_setup(AsyncWebServer *webServer)
{
    // Serve static files from SPIFFS
    if (!SPIFFS.begin(false))
    {
        DB_PRINTLN("SPIFFS mount failed");
        return;
    }
    else
    {
        DB_PRINTLN("SPIFFS mount success");
    }
    webServer->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    webServer->onNotFound([](AsyncWebServerRequest *request)
                         { request->send(404, "text/plain", "FileNotFound"); });
#ifdef SPIFFSEDITOR
    httpServer->addHandler(new SPIFFSEditor(SPIFFS));
#endif // SPIFFSEDITOR
}