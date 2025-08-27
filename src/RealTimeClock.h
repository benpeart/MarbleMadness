#ifndef REALTIMECLOCK_H
#define REALTIMECLOCK_H

#ifdef TIME

void rtc_setup();
void drawClock();
const char *getClockFace(int clockFace);
int setClockFace(const char *clockFace);
int setClockColor(const CRGB clockColor);

extern int clockFaces; // total number of valid face names in table

#endif // TIME

#endif // REALTIMECLOCK_H
