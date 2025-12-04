#ifndef DISPLAY_NUMBERS_H
#define DISPLAY_NUMBERS_H

void drawColon1x5(int xOffset, int yOffset, setLEDFunction setLED);
void drawDigit3x5(int digit, int xOffset, int yOffset, setLEDFunction setLED);
void drawTime17x5(int n1, int n2, int n3, int n4, int xOffset, int yOffset, setLEDFunction setLED);

#endif // DISPLAY_NUMBERS_H
