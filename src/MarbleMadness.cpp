#include "main.h"
#include "debug.h"
#include "render.h"
#include "MarbleMadness.h"

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
