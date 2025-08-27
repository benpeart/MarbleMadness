#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h> // for storing settings in the ESP32 EEPROM
#include "main.h"
#include "render.h" // for CRGB

// allow access to the preferences for additional settings
extern Preferences preferences;

// persisted settings are stored in the EEPROM
typedef struct
{
    int mode;
    int speed;
    int brightness;
#ifdef TIME
    int clockFace; // Index of current clock face in table
    CRGB clockColor;
#endif // TIME
} marblemadness_settings;
extern marblemadness_settings settings;

// load persisted settings
void settingsSetup();

// persist any changes to the settings
void settingsPersist();

#endif // SETTINGS_H