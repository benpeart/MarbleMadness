#include "main.h"
#include "debug.h"
#include "settings.h"
#include "render.h"

// With parallel updates for the LEDs so fast, we get flickering if we call
// FastLED.Show every loop. Maintain a 'dirty' bit so we know when to call Show.
boolean leds_dirty = true;

// led array which will be displayed
// have one unused space that we an return when something is out of bounds to make exception handling simpler
CRGB leds[NUM_LEDS + 1];

#define BOTTOM_RIGHT

uint16_t XY(uint16_t x, uint16_t y)
{
  // any out of bounds address maps to the hidden pixel
  if ((x < 0) || (x >= NUM_COLS) || (y < 0) || (y >= NUM_ROWS))
    return OUTOFBOUNDS;

  // Calculate the LED index based on a serpintine mapping
#ifdef BOTTOM_RIGHT
  // (0,0) is top left (index 360), (18,18) is bottom right (index = 0)
  // each strip is a single row of the XY matrix
  int base_index;

  if (y % 2 == 0)
  {
    // Even row: right to left
    base_index = y * NUM_COLS + x;
  }
  else
  {
    // Odd row: left to right
    base_index = y * NUM_COLS + (NUM_COLS - 1 - x);
  }

  // Reverse the index
  return (NUM_ROWS * NUM_COLS - 1) - base_index;
#endif // BOTTOM_RIGHT
#ifdef TOP_LEFT
  // (0,0) is top left (index 0), (18,18) is bottom right (index = 360)
  // each strip is a single row of the XY matrix
  if (y % 2 == 0)
  {
    // Even row: left-to-right
    return y * NUM_COLS + x;
  }
  else
  {
    // Odd row: right-to-left
    return y * NUM_COLS + (NUM_COLS - 1 - x);
  }

  return OUTOFBOUNDS; // should never get here
#endif                // TOP_LEFT
}

#ifdef COMPLEX_SHAPE
//
// I have implemented two different X,Y mapping modes. The first pairs adjacent strips of LEDs into a single x coordinate
// as they are paired in the triangle columns. The second (WIDERTHANTALLER) maps each strip to a separate x coordinate.
//
#define STRIP_0_NUM_COLS 6
#define STRIP_1_NUM_COLS 4
#define STRIP_2_NUM_COLS 4
#define STRIP_3_NUM_COLS 6

int XY(int x, int y)
{
  const uint8_t XYTable[NUM_ROWS * NUM_COLS / 2] = {
      //
      // This array helps translate from a (x,y) coordinate to the correct offset in the leds[] array.
      // To get the best visual results, we are combining columns as the triangles are interleaved
      // vertically. This gives us an effective resolution of (20 x 39) pixels with missing corners
      // where the hexaqgon shape doesn't fill the square. Any (x, y) coordinates that aren't valid
      // LED positions, return a value that is within the leds[] array but doesn't have a corresponding
      // physical LED to make error handling simpler.
      //
      255, 255, 255, 255, 255, 255, 255, 255, 255, 19,
      255, 255, 255, 255, 255, 255, 255, 255, 57, 20,
      255, 255, 255, 255, 255, 255, 255, 93, 58, 18,
      255, 255, 255, 255, 255, 255, 127, 94, 56, 21,
      255, 255, 255, 255, 255, 15, 128, 92, 59, 17,
      255, 255, 255, 255, 45, 16, 126, 95, 55, 22,
      255, 255, 255, 73, 46, 14, 129, 91, 60, 16,
      255, 255, 99, 74, 44, 17, 125, 96, 54, 23,
      255, 123, 100, 72, 47, 13, 130, 90, 61, 15,
      145, 124, 98, 75, 43, 18, 124, 97, 53, 24,
      146, 122, 101, 71, 48, 12, 131, 89, 62, 14,
      144, 125, 97, 76, 42, 19, 123, 98, 52, 25,
      147, 121, 102, 70, 49, 11, 132, 88, 63, 13,
      143, 126, 96, 77, 41, 20, 122, 99, 51, 26,
      148, 120, 103, 69, 50, 10, 133, 87, 64, 12,
      142, 127, 95, 78, 40, 21, 121, 100, 50, 27,
      149, 119, 104, 68, 51, 9, 134, 86, 65, 11,
      141, 128, 94, 79, 39, 22, 120, 101, 49, 28,
      150, 118, 105, 67, 52, 8, 135, 85, 66, 10,
      140, 129, 93, 80, 38, 23, 119, 102, 48, 29,
      151, 117, 106, 66, 53, 7, 136, 84, 67, 9,
      139, 130, 92, 81, 37, 24, 118, 103, 47, 30,
      152, 116, 107, 65, 54, 6, 137, 83, 68, 8,
      138, 131, 91, 82, 36, 25, 117, 104, 46, 31,
      153, 115, 108, 64, 55, 5, 138, 82, 69, 7,
      137, 132, 90, 83, 35, 26, 116, 105, 45, 32,
      154, 114, 109, 63, 56, 4, 139, 81, 70, 6,
      136, 133, 89, 84, 34, 27, 115, 106, 44, 33,
      155, 113, 110, 62, 57, 3, 140, 80, 71, 5,
      135, 134, 88, 85, 33, 28, 114, 107, 43, 34,
      255, 112, 111, 61, 58, 2, 141, 79, 72, 4,
      255, 255, 87, 86, 32, 29, 113, 108, 42, 35,
      255, 255, 255, 60, 59, 1, 142, 78, 73, 3,
      255, 255, 255, 255, 31, 30, 112, 109, 41, 36,
      255, 255, 255, 255, 255, 0, 143, 77, 74, 2,
      255, 255, 255, 255, 255, 255, 111, 110, 40, 37,
      255, 255, 255, 255, 255, 255, 255, 76, 75, 1,
      255, 255, 255, 255, 255, 255, 255, 255, 39, 38,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 0};

  // Compute the index into the lookup table taking into account the fact that the table
  // only contains 1/2 the columns to save space. The right have are just mirrored from
  // the left half.
  uint16_t tableIndex = (y * NUM_COLS / 2);
  if (x < NUM_COLS / 2)
  {
    tableIndex += x;
  }
  else
  {
    tableIndex += NUM_COLS - x - 1;
  }
  uint16_t stripOffset = XYTable[tableIndex];
  uint8_t strip = 0;

  // if we are returning an out of bounds value for an individual strip, return the first hidden LED instead
  // to make error handling easier.
  if (stripOffset >= NUM_LEDS_PER_STRIP)
    return OUTOFBOUNDS;

  if (x < STRIP_0_NUM_COLS)
  {
    strip = 0;
  }
  else if (x < STRIP_0_NUM_COLS + STRIP_1_NUM_COLS)
  {
    strip = 1;
  }
  else if (x < STRIP_0_NUM_COLS + STRIP_1_NUM_COLS + STRIP_2_NUM_COLS)
  {
    strip = 2;
  }
  else if (x < STRIP_0_NUM_COLS + STRIP_1_NUM_COLS + STRIP_2_NUM_COLS + STRIP_3_NUM_COLS)
  {
    strip = 3;
  }

  return strip * NUM_LEDS_PER_STRIP + stripOffset;
}
#endif // COMPLEX_SHAPE

#if 0
TRANSITION_TYPE current_transition = RANDOM;
CRGB *source_frame;
void set_transition(TRANSITION_TYPE type, CRGB *leds
{
  current_transition = type;
  source_frame = leds
}

void transition_drawframe()
{
  switch (current_transition)
  {
  case MARBLE_ROLLER:
    marbleroller_drawframe();
    break;
  case ROLL_FROM_LEFT:
    rollfromleft_drawframe();
    break;
  case ROLL_FROM_RIGHT:
    rollfromright_drawframe();
    break;
  case SIMPLE_CUT:
    simplecut_drawframe();
    break;
  case CROSSFADE:
    crossfade_drawframe();
    break;
  case SLIDE:
    slide_drawframe();
    break;
  case WIPE:
    wipe_drawframe();
    break;
  case PUSH:
    push_drawframe();
    break;
  case FALL:
    fall_drawframe();
    break;
  }
}
#endif
