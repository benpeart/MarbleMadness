#include "main.h"
#include "debug.h"
//#include "settings.h"
#include "render.h"
#include "MarbleMadness.h"

void mode_marbleroller()
{
#if 0
// we're going to blend between frames so need an array of LEDs for both frames
// then we'll blend them together into the 'output' leds array which will be displayed
CRGB leds2[NUM_STRIPS * NUM_LEDS_PER_STRIP];
CRGB leds3[NUM_STRIPS * NUM_LEDS_PER_STRIP];

#define DEFAULT_MILLIS 500
#define MIN_MILLIS 0
#define MAX_MILLIS (4 * DEFAULT_MILLIS)

#define VIEWPORT_HEIGHT 10  // the height of the 'viewport' triangle
#define TRIANGLE_COLUMNS 19 // the width of the base of the 'viewport' triange

// Use qsuba for smooth pixel colouring and qsubd for non-smooth pixel colouring
#define qsubd(x, b) ((x > b) ? b : 0)     // Digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b) ((x > b) ? x - b : 0) // Analog Unsigned subtraction macro. if result <0, then => 0

#define SCREENSAVER_DELAY 60000 // 1 minute = 60,000 milliseconds


static int time_to_enter_screensaver_mode = 0; // start in screensaver mode
    static boolean first_array = true;
    static int time_of_last_frame = 0;
    static boolean drawframe = true; // start by drawing the first frame
    static boolean blendframes = false;
    static bool init = false;

    // do a one time init
    if (!init)
    {
        init = true;
        chooseNextDiskPalette(gKTargetPalette);
    }

    EVERY_N_SECONDS(SECONDS_PER_PALETTE)
    {
        chooseNextDiskPalette(gKTargetPalette);
    }

    EVERY_N_MILLIS(10)
    {
        nblendPaletteTowardPalette(gCurrentDiskPalette, gKTargetPalette, 12);
    }

    int time = millis();
    int ms_between_frames = MAX_MILLIS - map(settings.speed, MIN_SPEED, MAX_SPEED, MIN_MILLIS, MAX_MILLIS);

    // if it is time to enter screen saver mode
    if (time >= time_to_enter_screensaver_mode)
    {
        // if it is time to draw the next frame
        if (time >= time_of_last_frame + ms_between_frames)
        {
            // update our offsets
            ++TriangleDisk;
            ++SquareDisk;
            drawframe = true;
        }
    }

    // draw the next frame into the correct led array
    // When changing modes (esp through 'off') the leds are flagged dirty so that they
    // get redrawn, without checking for leds_dirty here, we wouldn't redraw
    // MarbleMadness unless there was movement (via the knobs) or if we were already in
    // screen saver mode.
    if (drawframe || leds_dirty)
    {
        // draw the next frame of MarbleMadness
        drawPaletteFrame(first_array ? leds2 : leds3, &TriangleDisk, &SquareDisk);
        time_of_last_frame = time;
        first_array = !first_array;
        blendframes = true;
        drawframe = false;
    }

    // smoothly blend from one frame to the next
    if (blendframes)
    {
        EVERY_N_MILLISECONDS(5)
        {
            // ratio is the percentage of time remaining for this frame mapped to 0-255
            fract8 ratio = map(time, time_of_last_frame, time_of_last_frame + ms_between_frames, 0, 255);
            if (ratio >= 250)
                blendframes = false;
            if (!first_array)
                ratio = 255 - ratio;

            // mix the 2 arrays together
            blend(leds2, leds3, leds, NUM_STRIPS * NUM_LEDS_PER_STRIP, ratio);
            leds_dirty = true;
        }
    }
#endif    
}

#ifdef DEBUG
// test the wiring and ensure all pixels light up correctly
void mode_test()
{
  static int index = 0;

  EVERY_N_MILLISECONDS(50)
  {
    // erase the last pixel
    leds[index] = CRGB::Black; // off

    // move to the next pixel
    if (++index >= NUM_STRIPS * NUM_LEDS_PER_STRIP)
      index = 0;
    DB_PRINTLN(index);

    // light up the next pixel
    leds[index] = CRGB::Red;

    leds_dirty = true;
  }
}

// loop through all pixels via x,y coordinates making sure they all get mapped correctly
void mode_xy_test()
{
  static int x = -1, y = 0;
  int index;

  EVERY_N_MILLISECONDS(75)
  {
    // erase the last pixel
    index = XY(x, y);
    leds[index] = CRGB::Black; // off

    // move to the next pixel
    if (++x >= NUM_COLS)
    {
      x = 0;
      if (++y >= NUM_ROWS)
        y = 0;
    }
    index = XY(x, y);

    DB_PRINT("x = ");
    DB_PRINT(x);
    DB_PRINT(" y = ");
    DB_PRINT(y);
    DB_PRINT(" index = ");
    DB_PRINTLN(index);

    // light up the next pixel
    leds[index] = CRGB::Red;

    leds_dirty = true;
  }
}

#endif // DEBUG
