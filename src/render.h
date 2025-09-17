#ifndef RENDER_H
#define RENDER_H

// https://github.com/FastLED/FastLED
// #define FASTLED_ESP32_I2S true // causes white flashes across several strips at the same time
// #define FASTLED_ESP32_FLASH_LOCK 1   // force flash operations to wait until the show() is done. (doesn't fix the hang when updating the firmware while displaying LEDs)
// #define FASTLED_ALL_PINS_HARDWARE_SPI
#include <FastLED.h>

// With parallel updates for the LEDs so fast, we get flickering if we call
// FastLED.Show every loop. Maintain a 'dirty' bit so we know when to call Show.
extern boolean leds_dirty;

#define NUM_STRIPS 1
#define NUM_LEDS_PER_STRIP 361
extern CRGB leds[];

// The width and height of the XY coordinate system. The corners outside the hexagon
// are 'missing' so can't display any values assigned to them.
#define NUM_COLS 19
#define NUM_ROWS 19
#define WIDTH NUM_COLS
#define HEIGHT NUM_ROWS
#define NUM_LEDS (NUM_STRIPS * NUM_LEDS_PER_STRIP)
#define OUTOFBOUNDS (NUM_STRIPS * NUM_LEDS_PER_STRIP)

// function to map a x, y coordinate to the index into the led array
// origin (x = 0, y = 0) is at top left
uint16_t XY(uint16_t x, uint16_t y);

typedef enum
{
    MARBLE_ROLLER,      // Marbles roll in one at a time to fill the screen with new image
    ROLL_FROM_LEFT,     // Marbles roll in one column at a time to fill the screen with the new image left to right.
    ROLL_FROM_RIGHT,    // Marbles roll in one column at a time to fill the screen with the new image right to left.
    SIMPLE_CUT,         // Simple hard cut from one image to the next.
    CROSSFADE,          // Smoothly blends from one image to the next.
    SLIDE,              // Slides the new image over the old one.
    WIPE,               // Wipes the new image over the old one.
    PUSH,               // Pushes the old image off the display with the new one.
    FALL,               // New image falls down over the old one.
    RANDOM,             // Randomly selects a transition for each
} TRANSITION_TYPE;

void set_transition(TRANSITION_TYPE type, CRGB *leds);
void transition_drawframe();

#endif // RENDER_H
