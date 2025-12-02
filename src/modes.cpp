#include "main.h"
#include "debug.h"
#include "settings.h"
#include "modes.h"

#include "MarbleMadness.h"
#include "bounce.h"
#include "Ringer.h"
#include "life.h"
#include "MarbleRoller.h"
#include "MarbleTrack.h"
#include "pachinko.h"
#include "physicsRoller.h"
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
    void (*enterFunc)(void);            // pointer to the function to call when entering the mode
    void (*renderFunc)(void);           // pointer to the function that will render the mode
    void (*exitFunc)(void);             // pointer to the function to call when exiting the mode
    const char modeName[MAX_MODE_NAME]; // name of the mode to use in the UI and REST APIs
    int showInRESTAPI : 1;              // flag if the mode should be shown in the REST APIs
};

// This look up table lists each of the display/animation drawing functions
MarbleMadnessMode MarbleMadnessLUT[]{
    {NULL, mode_marbleroller, NULL, "MarbleRoller", true},
    {marbletrack_enter, marbletrack_loop, NULL, "MarbleTrack", true},
    {physicsRoller_enter, physicsRoller_loop, physicsRoller_leave, "PhysicsRoller", true},
    {ringer_enter, ringer_loop, ringer_leave, "Ringer", true},
    {bounce_enter, bounce_loop, bounce_leave, "Bounce", true},
    {pachinko_enter, pachinko_loop, pachinko_leave, "Pachinko", true},
    {life_enter, life_loop, life_leave, "Life", true},
    {NULL, mode_xy_fire, NULL, "Fire", true},
    {NULL, mode_xy_matrix, NULL, "Matrix", true},
#ifdef DEBUG
    {NULL, mode_xy_test, NULL, "xy_test", true},
    {NULL, mode_test, NULL, "test", true},
#endif
    {NULL, mode_off, NULL, "off", false} // make it obvious we're entering 'regular' modes
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
                // call the exit function for the old mode iff it is valid
                if (settings.mode >= 0 && settings.mode < marblemadnessModes && MarbleMadnessLUT[settings.mode].exitFunc)
                {
                    (*MarbleMadnessLUT[settings.mode].exitFunc)();
                }

                // output the new mode name and clear the led strips for the new mode
                settings.mode = x;
                DB_PRINTF("setMarbleMadnessMode: %s\r\n", MarbleMadnessLUT[settings.mode].modeName);
                FastLED.clear(true);
                leds_dirty = true;

                // call the enter function for the new mode if it exists
                if (MarbleMadnessLUT[x].enterFunc)
                {
                    (*MarbleMadnessLUT[x].enterFunc)();
                }
            }
            break;
        }
    }
}

const char *getMarbleMadnessMode(int mode)
{
    if (mode < 0 || mode >= marblemadnessModes)
        return MarbleMadnessLUT[0].modeName;

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
