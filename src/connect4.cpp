#include "main.h"
#include "debug.h"
#include "render.h"
#include "physics.h"
#include "connect4.h"
#include "settings.h"
#include "RealTimeClock.h"

// Track our marbles so we can move them around
#define CLOCK_HEIGHT 5
#define CLOCK_WIDTH 17
#define MARBLE_COUNT (CLOCK_HEIGHT * CLOCK_WIDTH)
static bool clockColors[MARBLE_COUNT];
static b2BodyId marbles[MARBLE_COUNT];
static b2BodyId floorId;

// position the marbles to show the current time
static void ResetMarbles()
{
    // Move marbles into new positions above the top of the display
    for (int r = 0; r < CLOCK_HEIGHT; ++r)
    {
        for (int c = 0; c < CLOCK_WIDTH; ++c)
        {
            int index = r * CLOCK_WIDTH + c;
#ifdef DEBUG
            if (!b2Body_IsValid(marbles[index]))
            {
                DB_PRINTF("Invalid marble at index %d at %s:%d\n", index, __FILE__, __LINE__);
                continue;
            }
#endif // DEBUG
            b2Vec2 position = b2Vec2{(float)c, (float)(NUM_ROWS + r + 1)};
            b2Rot rotation = b2MakeRot(0.0f);
            b2Body_SetTransform(marbles[index], position, rotation); // Move to new location
            float vy = (random(-100, 0)) / 50.0f;
            b2Body_SetLinearVelocity(marbles[index], (b2Vec2){0, vy});
            b2Body_SetAngularVelocity(marbles[index], 0.0f); // Stop spin
        }
    }

    // now draw the clock into the marbleColors array
    memset(clockColors, 0, sizeof(clockColors));
    drawDigitalClock(0, 0, [](int x, int y)
                     {
                        // invert y to match physics marble layout
                int index = (CLOCK_HEIGHT - y - 1) * CLOCK_WIDTH + x;
#ifdef DEBUG                
                if (index >= 0 && index < MARBLE_COUNT)
#endif // DEBUG
                {
                    clockColors[index] = true;
                } });
}

static void setupWorld()
{
    // create the world
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, -9.8f};
    world = b2CreateWorld(&worldDef);

    if (B2_IS_NULL(world))
    {
        DB_PRINTLN("ERROR: Failed to create Box2D world");
        return;
    }

    // create a floor that we will remove as needed to let marbles fall
    int startY = (NUM_ROWS - CLOCK_HEIGHT) / 2;
    floorId = CreateLine(0, startY, NUM_COLS, startY);

    // create walls to create columns
    for (int c = 0; c <= NUM_COLS; ++c)
    {
        CreateLine((float)c - 0.5f, 0.0f, (float)c - 0.5f, (float)NUM_ROWS * 2.0f, (float)0.1f);
    }

    // Create the marble objects
    for (int index = 0; index < MARBLE_COUNT; ++index)
    {
        marbles[index] = CreateCircle(0.0f, 0.0f, 0.5f);
#ifdef DEBUG
        if (!b2Body_IsValid(marbles[index]))
        {
            DB_PRINTF("ERROR: Failed to create marble at index %d\n", index);
        }
#endif // DEBUG
    }
}

void connect4_enter()
{
    // Initializie physics world and start physics task
    DB_PRINTLN("Entering Connect4 mode");
    setupWorld();
    physics_enter();

    // Small delay to ensure physics task is running
    vTaskDelay(pdMS_TO_TICKS(100));

    // position the marbles to show the current time (after physics task is running)
    if (xSemaphoreTake(worldMutex, portMAX_DELAY))
    {
        ResetMarbles();
        xSemaphoreGive(worldMutex);
    }
}

void connect4_leave()
{
    physics_leave();
    DB_PRINTLN("Leaving Bounce mode");
}

void connect4_loop()
{
    // ~60 FPS
    EVERY_N_MILLIS(16)
    {
        // Draw marbles at their current positions
        FastLED.clear();
        for (int i = 0; i < MARBLE_COUNT; ++i)
        {
#ifdef DEBUG
            if (!b2Body_IsValid(marbles[i]))
            {
                DB_PRINTF("Invalid marble at index %d at %s:%d\n", i, __FILE__, __LINE__);
                continue;
            }
#endif // DEBUG
       // Center the clock in the LED display
            b2Vec2 position = b2Body_GetPosition(marbles[i]);
            int gx = (NUM_COLS - CLOCK_WIDTH) / 2 + (int)lroundf(position.x);
            int gy = NUM_ROWS - (int)lroundf(position.y);
            leds[XY(gx, gy)] = clockColors[i] ? settings.clockColor : CRGB::Black;
        }

        leds_dirty = true;
    }

    // if the floor is in place, check if we need to remove it to let the marbles fall
    if (b2Body_IsValid(floorId))
    {
        // reset marble positions every time the minute changes
        static int lastMinute = -1;
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            int currentMinute = timeinfo.tm_min;

            if (currentMinute != lastMinute)
            {
                lastMinute = currentMinute;

                // make sure we can get the world mutex before changing the world
                if (xSemaphoreTake(worldMutex, portMAX_DELAY))
                {
                    // remove the floor to let marbles fall
                    b2DestroyBody(floorId);
                    floorId = B2_NULL_ID;

                    // give each marble a small downward velocity to start them falling
                    for (int i = 0; i < MARBLE_COUNT; ++i)
                    {
                        float vy = (random(-100, 0)) / 50.0f;
                        b2Body_SetLinearVelocity(marbles[i], (b2Vec2){0, vy});
                    }

                    xSemaphoreGive(worldMutex);
                }
            }
        }
    }
    else
    {
        // check to see if all marbles are below the visible area
        bool allBelow = true;
        for (int i = 0; i < MARBLE_COUNT; ++i)
        {
#ifdef DEBUG
            if (!b2Body_IsValid(marbles[i]))
            {
                DB_PRINTF("Invalid marble at index %d at %s:%d\n", i, __FILE__, __LINE__);
                continue;
            }
#endif // DEBUG
            b2Vec2 position = b2Body_GetPosition(marbles[i]);
            if (lroundf(position.y) >= 0)
            {
                allBelow = false;
                break;
            }
        }

        if (allBelow)
        {
            // make sure we can get the world mutex before recreating the floor
            if (xSemaphoreTake(worldMutex, portMAX_DELAY))
            {
                // recreate the floor
                int startY = (NUM_ROWS - CLOCK_HEIGHT) / 2;
                floorId = CreateLine(0, startY, NUM_COLS, startY);

                ResetMarbles();
                xSemaphoreGive(worldMutex);
            }
        }
    }
}
