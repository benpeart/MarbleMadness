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
    {0x07, 0x04, 0x1F}, // 4
    {0x17, 0x15, 0x1D}, // 5
    {0x1F, 0x15, 0x1D}, // 6
    {0x01, 0x01, 0x1F}, // 7
    {0x1F, 0x15, 0x1F}, // 8
    {0x17, 0x15, 0x1F}  // 9
};

void drawDigit3x5(int digit, int xOffset, int yOffset, setLEDFunction setLED)
{
    for (int col = 0; col < 3; ++col)
    {
        uint8_t bits = FONT3x5[digit][col];
        for (int row = 0; row < 5; ++row)
        {
            if (bits & (1 << row))
            {
                setLED(xOffset + col, yOffset + row);
            }
        }
    }
}

void drawColon1x5(int xOffset, int yOffset, setLEDFunction setLED) {
    // Dots at rows 1 and 3 within the 5-row block
    setLED(xOffset, yOffset + 1);
    setLED(xOffset, yOffset + 3);
}

void displayNumbers(int n1, int n2, int n3, int n4, setLEDFunction setLED)
{
#ifdef DEBUG
    // do some sanity checking
    if (NULL == setLED)
    {
        DB_PRINTLN("displayNumbers called with NULL setLED function pointer");
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
    drawDigit3x5(n1, x, startY, setLED); x += 3; x += 1;
    drawDigit3x5(n2, x, startY, setLED); x += 3; x += 1;
    drawColon1x5(    x, startY, setLED); x += 1; x += 1;
    drawDigit3x5(n3, x, startY, setLED); x += 3; x += 1;
    drawDigit3x5(n4, x, startY, setLED);
}
