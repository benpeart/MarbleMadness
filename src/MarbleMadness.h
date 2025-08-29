#ifndef MARBLEMADNESS_H
#define MARBLEMADNESS_H

#ifdef DEBUG
// test the wiring and ensure all pixels light up correctly
// Q: Why does led[300] ==> led[312] not light up?
// A: Our strips aren't the same length (156 vs 144) so the shorter strips (1 and 2)
// have extra leds[x] positions that don't have physical LEDs.
void mode_test();

// loop through all pixels via x,y coordinates making sure they all get mapped correctly
void mode_xy_test();
#endif // DEBUG

#endif // MARBLEMADNESS_H
