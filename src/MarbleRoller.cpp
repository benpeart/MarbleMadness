#include "main.h"
#include "debug.h"
#include "settings.h"
#include "render.h"
#include "MarbleRoller.h"

#define DEFAULT_MILLIS 75
#define MIN_MILLIS 0
#define MAX_MILLIS (4 * DEFAULT_MILLIS)

void mode_marbleroller()
{
    EVERY_N_MILLIS_I(timer, DEFAULT_MILLIS)
    {
        timer.setPeriod(MAX_MILLIS - map(settings.speed, MIN_SPEED, MAX_SPEED, MIN_MILLIS, MAX_MILLIS));
    }
}
