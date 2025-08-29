#include "main.h"
#include "debug.h"
#include "settings.h"
#include "modes.h"

#include "MarbleMadness.h"
#include "MarbleRoller.h"
#include "XYfire.h"
#include "xymatrix.h"

#ifdef TIME
#include "RealTimeClock.h"
#endif // TIME

// maximum lenth of a valid mode name
#define MAX_MODE_NAME 64

// Data structure to represent each mode the MarbleMadness can be in.
struct MarbleMadnessMode
{
    void (*renderFunc)(void);           // pointer to the function that will render the mode
    const char modeName[MAX_MODE_NAME]; // name of the mode to use in the UI and REST APIs
    bool showInRESTAPI;                 // flag if the mode should be shown in the REST APIs
};

// This look up table lists each of the display/animation drawing functions
MarbleMadnessMode MarbleMadnessLUT[]{
    {mode_marbleroller, "MarbleRoller", true},
    {mode_xy_fire, "Fire", true},
    {mode_xy_matrix, "Matrix", true},
#ifdef DEBUG
    {mode_xy_test, "xy_test", true},
    {mode_test, "test", true},
#endif
    {mode_off, "off", false} // make it obvious we're entering 'regular' modes
};
uint8_t marblemadnessModes = (sizeof(MarbleMadnessLUT) / sizeof(MarbleMadnessLUT[0])); // total number of valid modes in table

void marbleMadnessModeRender()
{
    // call the render function for the current mode
    (*MarbleMadnessLUT[settings.mode].renderFunc)();
}

void setMarbleMadnessMode(const char *newMode)
{
    for (int x = 0; x < marblemadnessModes; x++)
    {
        if (String(MarbleMadnessLUT[x].modeName).equalsIgnoreCase(newMode))
        {
            // if the mode changed
            if (settings.mode != x)
            {
                // output the new mode name and clear the led strips for the new mode
                settings.mode = x;
                DB_PRINTF("setMarbleMadnessMode: %s\r\n", MarbleMadnessLUT[settings.mode].modeName);
                FastLED.clear(true);
                leds_dirty = true;
            }
            break;
        }
    }
}

const char *getMarbleMadnessMode(int mode)
{
    if (mode < 0 || mode >= marblemadnessModes)
        return NULL;

    return MarbleMadnessLUT[mode].modeName;
}

bool getMarbleMadnessModeShowInRESTAPI(int mode)
{
    if (mode < 0 || mode >= marblemadnessModes)
        return false;

    return MarbleMadnessLUT[mode].showInRESTAPI;
}

// All Pixels off
void mode_off()
{
    // nothing to see here... (the pixels got cleared by the button press)
}
