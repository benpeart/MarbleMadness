#include "main.h"
#include "debug.h"
#include "render.h"
#include "displaynumbers.h"

static const uint8_t FONT3x5[10][3] = {
    // Each entry = 3 columns; bits 0..4 = rows top..bottom
    {0x1F, 0x11, 0x1F}, // 0
    {0x00, 0x00, 0x1F}, // 1
    {0x19, 0x15, 0x13}, // 2
    {0x15, 0x15, 0x1B}, // 3
    {0x03, 0x07, 0x1F}, // 4
    {0x17, 0x15, 0x1D}, // 5
    {0x1F, 0x15, 0x1D}, // 6
    {0x01, 0x01, 0x1F}, // 7
    {0x1F, 0x15, 0x1F}, // 8
    {0x17, 0x15, 0x1F}  // 9
};

void drawDigit3x5(int digit, int xOffset, int yOffset, getColor color)
{
    if (digit < 0 || digit > 9)
        return;
    for (int col = 0; col < 3; ++col)
    {
        uint8_t bits = FONT3x5[digit][col];
        for (int row = 0; row < 5; ++row)
        {
            if (bits & (1 << row))
            {
                int index = XY(xOffset + col, yOffset + row);
                leds[index] = color(leds[index]);
            }
        }
    }
}

void drawColon1x5(int xOffset, int yOffset, getColor color) {
    // Dots at rows 1 and 3 within the 5-row block
    int index;
    index = XY(xOffset, yOffset + 1);
    leds[index]= color(leds[index]);
    index = XY(xOffset, yOffset + 3);
    leds[index]= color(leds[index]);
}

void displayNumbers(int n1, int n2, int n3, int n4, getColor color)
{
#ifdef DEBUG
    // do some sanity checking
    if (NULL == color)
    {
        DB_PRINTLN("displayNumbers called with NULL color function pointer");
        return;
    }

    if (n1 < 0 || n1 > 9 || n2 < 0 || n2 > 9 || n3 < 0 || n3 > 9 || n4 < 0 || n4 > 9)
    {
        DB_PRINTF("\rdisplayNumbers called with number that is out of range (0-9): %d, %d, %d, %d\r\n", n1, n2, n3, n4);
        return;
    }
#endif

    // Center a 17-column layout in a 19-column grid
    const int totalW = 17;
    const int totalH = 5;
    const int startX = (NUM_COLS - totalW) / 2; // = 1
    const int startY = (NUM_ROWS - totalH) / 2; // = 7

    int x = startX;

    // HH : MM with single-column spaces and colon
    drawDigit3x5(n1, x, startY, color); x += 3; x += 1;
    drawDigit3x5(n2, x, startY, color); x += 3; x += 1;
    drawColon1x5(    x, startY, color); x += 1; x += 1;
    drawDigit3x5(n3, x, startY, color); x += 3; x += 1;
    drawDigit3x5(n4, x, startY, color);
}

#ifdef FIVEWIDE
// Each digit is 5 pixels wide × 7 pixels tall. Each row is a byte, where bits represent pixels.
const uint8_t font[10][7] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

void drawDigit(int digit, int xOffset, int yOffset, getColor color)
{
    for (int col = 0; col < 5; col++)
    {
        uint8_t column = font[digit][col];
        for (int row = 0; row < 7; row++)
        {
            if (column & (1 << row))
            {
                int index = XY(xOffset + col, yOffset + row);
                leds[index] = color(leds[index]);
            }
        }
    }
}

void drawColon(int xOffset, int yOffset, getColor color)
{
    int index = XY(xOffset, yOffset + 2);
    leds[index] = color(leds[index]);
    index = XY(xOffset, yOffset + 4);
    leds[index] = color(leds[index]);
}

void displayNumbers(int n1, int n2, int n3, int n4, getColor color)
{
#ifdef DEBUG
    // do some sanity checking
    if (NULL == color)
    {
        DB_PRINTLN("displayNumbers called with NULL color function pointer");
        return;
    }

    if (n1 < 0 || n1 > 9 || n2 < 0 || n2 > 9 || n3 < 0 || n3 > 9 || n4 < 0 || n4 > 9)
    {
        DB_PRINTF("\rdisplayNumbers called with number that is out of range (0-9): %d, %d, %d, %d\r\n", n1, n2, n3, n4);
        return;
    }
#endif

    int startX = 2;     // Center horizontally
    int startY = 6 + 6; // Center vertically

    int spacing = 6; // Space between digits

    drawDigit(n1, startX + 0 * spacing, startY, color);
    drawDigit(n2, startX + 1 * spacing, startY, color);
    drawColon(startX + 2 * spacing - 1, startY, color);
    drawDigit(n3, startX + 2 * spacing, startY, color);
    drawDigit(n4, startX + 3 * spacing, startY, color);
}
#endif // FIVEWIDE
