#ifndef REALTIMECLOCK_H
#define REALTIMECLOCK_H

#ifdef TIME

void rtc_setup();
void drawClock();
const char *getClockFace(int clockFace);
int setClockFace(const char *clockFace);
CRGB setClockColor(const CRGB clockColor);

extern int clockFaces; // total number of valid face names in table

void drawDigitalClock(int xOffset, int yOffset, setLEDFunction setLED);

#endif // TIME

#endif // REALTIMECLOCK_H
