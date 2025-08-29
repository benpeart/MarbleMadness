#ifndef MODES_H
#define MODES_H

void marbleMadnessModeRender();
void setMarbleMadnessMode(const char *newMode);
const char *getMarbleMadnessMode(int mode);
bool getMarbleMadnessModeShowInRESTAPI(int mode);
void mode_off();

extern uint8_t marblemadnessModes; // total number of valid modes in the LUT

#endif // MODES_H
