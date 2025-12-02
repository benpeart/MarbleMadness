#ifndef DISPLAY_NUMBERS_H
#define DISPLAY_NUMBERS_H

typedef void (*setLEDFunction)(int x, int y);
void drawDigit3x5(int digit, int xOffset, int yOffset, setLEDFunction setLED);
void displayNumbers(int n1, int n2, int n3, int n4, setLEDFunction setLED);

#endif // DISPLAY_NUMBERS_H
