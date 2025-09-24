#include "main.h"
#include "debug.h"
#include "settings.h"
#include "modes.h"

#ifdef WIFI
#include "WiFiHelpers.h"

#ifdef REST
#include <AsyncJson.h>
#include <ArduinoJson.h>
#endif // REST

#ifdef TIME
#include "RealTimeClock.h"
#endif // TIME

#endif // WIFI

//
// GLOBAL PIN DECLARATIONS -------------------------------------------------
//
// setup our LED strips for parallel output using FastLED
#define LED_STRIP_PIN_1 18
#define LED_STRIP_PIN_2 27
#define LED_STRIP_PIN_3 26
#define LED_STRIP_PIN_4 25
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#ifdef REST
// update the FastLED brightness
void setBrightness(int brightness)
{
  // constrain our total brightness from MIN_BRIGHTNESS to MAX_BRIGHTNESS so it doesn't get too dark
  int newBrightness = constrain(brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);

  // adjust our brightness if it has changed
  if (newBrightness != settings.brightness)
  {
    settings.brightness = newBrightness;
    DB_PRINTF("new brightness = %d\r\n", settings.brightness);

    FastLED.setBrightness(dim8_raw(settings.brightness));
    leds_dirty = true;
  }
}

// update the speed
void setSpeed(int speed)
{
  // constrain our speed to valid range
  int newSpeed = constrain(speed, 0, MAX_SPEED);

  // adjust our speed if it has changed
  if (newSpeed != settings.speed)
  {
    settings.speed = newSpeed;
    DB_PRINTF("new speed = %d\r\n", settings.speed);
  }
}

void setRESTSettings(AsyncWebServerRequest *request, JsonVariant &json)
{
  const JsonObject &jsonObj = json.as<JsonObject>();

  DB_PRINTLN("REST setRESTSettings:");

  // update the brightness (if it was passed)
  JsonVariant brightness = jsonObj["brightness"];
  if (!brightness.isNull())
  {
    setBrightness((int)brightness);
  }

  JsonVariant speed = jsonObj["speed"];
  if (!speed.isNull())
  {
    setSpeed((int)speed);
  }

  const char *modeName = jsonObj["mode"];
  if (modeName)
  {
    setMarbleMadnessMode(modeName);
  }

#ifdef TIME
  const char *clockFace = jsonObj["clockFace"];
  if (clockFace)
  {
    setClockFace(clockFace);
  }

  JsonVariant clockColor = jsonObj["clockColor"];
  if (!clockColor.isNull())
  {
    uint32_t color;

    sscanf(clockColor, "#%06X", &color);
    setClockColor(CRGB(color));
  }
#endif // TIME

  request->send(200, "text/plain", "OK");
}

void getRESTSettings(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  String response;

  doc["mode"] = getMarbleMadnessMode(settings.mode);
  doc["brightness"] = settings.brightness;
  doc["speed"] = settings.speed;
#ifdef TIME
  doc["clockFace"] = getClockFace(settings.clockFace);
  char color[8];
  sprintf(color, "#%06X", settings.clockColor.r << 16 | settings.clockColor.g << 8 | settings.clockColor.b);
  doc["clockColor"] = color;
#endif // TIME

  serializeJson(doc, response);
  DB_PRINTLN("REST getRESTSettings: " + response);
  request->send(200, "text/json", response);
}

void getModes(AsyncWebServerRequest *request)
{
  // allocate the memory for the document
  JsonDocument doc;

  // create an empty array
  JsonArray array = doc.to<JsonArray>();

  // add the names
  for (int x = 0; x < marblemadnessModes; x++)
  {
    if (getMarbleMadnessModeShowInRESTAPI(x))
      array.add(getMarbleMadnessMode(x));
  }

  // serialize the array and send the result
  String response;
  serializeJson(doc, response);
  DB_PRINTLN("REST getModes: " + response);
  request->send(200, "text/json", response);
}

#ifdef TIME
void getFaces(AsyncWebServerRequest *request)
{
  // allocate the memory for the document
  JsonDocument doc;

  // create an empty array
  JsonArray array = doc.to<JsonArray>();

  // add the names
  for (int x = 0; x < clockFaces; x++)
  {
    array.add(getClockFace(x));
  }

  // serialize the array and send the result
  String response;
  serializeJson(doc, response);
  DB_PRINTLN("REST getFaces: " + response);
  request->send(200, "text/json", response);
}
#endif // TIME
#endif // REST

//
// SETUP FUNCTION -- RUNS ONCE AT PROGRAM START ----------------------------
//

void setup()
{
#ifdef DEBUG
  // 3 second delay for recovery
  delay(3000);

  Serial.begin(115200);
  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB port only
  DB_PRINTLN("\nStarting MarbleMadness on " + String(ARDUINO_BOARD));

  // debug info about the ESP32 we are running on
  DB_PRINTLN("ESP32 Chip Model: " + String(ESP.getChipModel()));
  DB_PRINTLN("ESP32 Chip Revision: " + String(ESP.getChipRevision()));
  DB_PRINTLN("ESP32 Chip Cores: " + String(ESP.getChipCores()));
  DB_PRINTLN("ESP32 CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
  DB_PRINTLN("ESP32 Flash Size: " + String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB");
  DB_PRINTLN("ESP32 Flash Speed: " + String(ESP.getFlashChipSpeed() / 1000000) + " MHz");
  DB_PRINTLN("ESP32 PSRAM Size: " + String(ESP.getPsramSize())); // Should print 8388608
  DB_PRINTLN("ESP32 Free PSRAM: " + String(ESP.getFreePsram())); // Shows available heap in PSRAM
#endif

  // initialize the settings from persistent storage
  settingsSetup();

#ifdef WIFI
  // connect to wifi or enter AP mode so it can be configured
  wifi_setup();

#ifdef REST
  // setup the REST API endpoints and handlers
  webServer.on("/api/settings", HTTP_GET, getRESTSettings);
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/api/settings", setRESTSettings);
  webServer.addHandler(handler);
  webServer.on("/api/modes", HTTP_GET, getModes);
#ifdef TIME
  webServer.on("/api/faces", HTTP_GET, getFaces);
#endif // TIME
#endif // REST
#endif // WIFI

  // initialize the random number generator using the ESP32 hardware RNG
  randomSeed(esp_random());

  // intialize the LED strips for parallel output
  FastLED.addLeds<LED_TYPE, LED_STRIP_PIN_1, COLOR_ORDER>(leds + 0 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(dim8_raw(settings.brightness));
  leds_dirty = true;

  // re-set the mode to ensure proper initialization
  int mode = settings.mode;
  settings.mode = -1; // force a change
  setMarbleMadnessMode(getMarbleMadnessMode(mode));
}

//
// LOOP FUNCTION -- RUNS OVER AND OVER FOREVER -----------------------------
//
void loop()
{
#ifdef WIFI
  // check that WiFi is still connected and reconnect if necessary
  wifi_loop();
#endif // WIFI

  // Render one frame in current mode. To control the speed of updates, use the
  // EVERY_N_MILLISECONDS(N) macro to only update the frame when it is needed.
  // Also be sure to set leds_dirty = true so that the updated frame will be displayed.
  marbleMadnessModeRender();

#ifdef TIME
  // draw the clock face (can be a null clock face - see mode_select_clock_face())
  drawClock();
#endif // TIME

  // if we have changes in the LEDs, show the updated frame
  //  if (leds_dirty)
  {
#ifdef DEBUG_SPINNER
    static const char *spinner = "|/-\\";
    static int spinner_index = 0;

    DB_PRINTF("\r%c", spinner[spinner_index]);
    spinner_index = (spinner_index + 1) % sizeof(spinner);
#endif // DEBUG_SPINNER
#ifdef DEBUG_FPS
    static unsigned long lastFPS = 0;
    static uint16_t frames = 0;
    frames++;

    // Once per second, print FPS
    EVERY_N_MILLISECONDS(1000)
    {
      Serial.print("FPS: ");
      Serial.println(frames);
      frames = 0;
    }
#endif // DEBUG_FPS

    leds_dirty = false; // clear the dirty flag before showing the frame or changes via asyncronous REST calls will fail to be drawn
    FastLED.show();
  }

  // persist any changes to the settings
  EVERY_N_SECONDS(5)
  {
    settingsPersist();
  }
}
