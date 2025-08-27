#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

// don't include components that require WiFi unless it is included
#define WIFI
#ifdef WIFI
#define DRD
#define OTA
#define REST
#define TIME
//#define SPIFFSEDITOR
#endif

// update the FastLED brightness based on our ambient and manual settings only if requested (using the right knob)
#define MIN_BRIGHTNESS 32  // the minimum brightness we want (above zero so it doesn't go completely dark)
#define MAX_BRIGHTNESS 255 // the max possible brightness
void setBrightness(int brightness);

// 'speed' ranges from 0-255 with the default speed being 128
#define MIN_SPEED 0
#define MAX_SPEED 255
void setSpeed(int speed);

#endif // MAIN_H
