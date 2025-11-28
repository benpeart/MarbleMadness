#include "main.h"
#include "settings.h"
#include "render.h"
#include "pachinko.h"
#include "physics.h"
#include "debug.h"

// Create Box2D world with gravity
#define MARBLE_COUNT 6
static b2BodyId marbles[MARBLE_COUNT];

#if 0
// Example 19x19 bit array (1 = LED on, 0 = LED off)
uint32_t pinPattern[HEIGHT] = {
    0b0000000000000000000,
    0b0010001000100010001,
    0b0000000000000000000,
    0b1000100010001000100,
    0b0000000000000000000,
    0b0010001000100010001,
    0b0000000000000000000,
    0b1000100010001000100,
    0b0000000000000000000,
    0b0010001000100010001,
    0b0000000000000000000,
    0b1000100010001000100,
    0b0000000000000000000,
    0b0010001000100010001,
    0b0000000000000000000,
    0b1000100010001000100,
    0b0000000000000000000,
    0b0010001000100010001,
    0b0000000000000000000};
#else
// Example 19x19 bit array (1 = LED on, 0 = LED off)
uint32_t pinPattern[HEIGHT] = {
    0b0000000000000000000,
    0b1000000000000000001,
    0b0100000000000000010,
    0b0010000000000000100,
    0b0001000000000001000,
    0b0000100000000010000,
    0b0000010000000100000,
    0b0000001000001000000,
    0b0000000100010000000,
    0b0000000010100000000,
    0b0000000000000000000,
    0b0100010001000100010,
    0b0000000000000000000,
    0b0001000100010001000,
    0b0000000000000000000,
    0b0100010001000100010,
    0b0000000000000000000,
    0b0001000100010001000,
    0b0000000000000000000};
#endif
// ----- Physics setup -----
static void setupWorld()
{
    // create the world
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, -9.8f};
    world = b2CreateWorld(&worldDef);

    //    CreateWall((float)WIDTH / 2.0f, -0.125f, (float)WIDTH + 0.5f, 0.25f);             // floor
    CreateWall(-0.25f, (float)HEIGHT / 2.0f, 0.25f, (float)HEIGHT + 2.0f);                  // left wall
    CreateWall((float)WIDTH - 1.0f + 0.25f, (float)HEIGHT / 2.0f, 0.25f, (float)HEIGHT + 2.0f);    // right wall

    // Create pins based on the pinPattern array
    for (uint8_t y = 0; y < HEIGHT; y++)
    {
        for (uint8_t x = 0; x < WIDTH; x++)
        {
            // Adjust the bit field logic to avoid mirroring on the x-axis
            if ((pinPattern[y] >> (WIDTH - 1 - x)) & 0x1)
            {
                // The coefficient of restitution (CoR) for a steel pachinko pin typically falls in the range of 0.80 to 0.85.
                CreateCircle((float)x, (float)(HEIGHT - y), 0.15f, 0.3f, 0.80f, b2_staticBody);
            }
        }
    }

    // Spawn marbles at random positions along the top row
    for (int i = 0; i < MARBLE_COUNT; ++i)
    {
        float x = (float)(random(0, WIDTH));
        marbles[i] = CreateCircle(x, (float)HEIGHT, 0.45f);

        // Give each an initial push
        float vx = (random(-100, 101)) / 50.0f; // ~[-2, 2] m/s
        float vy = (random(10, 151)) / 50.0f;   // ~[0.2, 3] m/s upward
        b2Body_SetLinearVelocity(marbles[i], (b2Vec2){vx, vy});
    }
}

void pachinko_enter()
{
    // Initialize physics world and start physics task
    DB_PRINTLN("Entering Pachinko mode");
    setupWorld();
    physics_enter();
}

void pachinko_leave()
{
    physics_leave();
    DB_PRINTLN("Leaving Pachiinko mode");
}

void pachinko_loop()
{
    // ~60 FPS
    EVERY_N_MILLIS(16)
    {
        CRGB colors[] = {
            CRGB::Red,      // Bold and warm
            CRGB::Green,    // Natural and vibrant
            CRGB::Blue,     // Cool and deep
            CRGB::Yellow,   // Bright and energetic
            CRGB::Purple,   // Rich and regal
            CRGB::Cyan,     // Tropical and fresh
            CRGB::Orange,   // Warm and punchy
            CRGB::Pink,     // Playful and vivid
            CRGB::LimeGreen // Electric and sharp
        };

        // Clear the LED array and redraw the pins
        FastLED.clear();
        for (uint8_t y = 0; y < HEIGHT; y++)
        {
            for (uint8_t x = 0; x < WIDTH; x++)
            {
                if ((pinPattern[y] >> (WIDTH - 1 - x)) & 0x1)
                {
                    leds[XY(x, y)] = CRGB(0x161616);
                }
            }
        }

        // Draw marbles at their current positions
        bool marblesVisible = false;
        for (int i = 0; i < MARBLE_COUNT; ++i)
        {
            if (!b2Body_IsValid(marbles[i]))
                continue;

            b2Vec2 position = b2Body_GetPosition(marbles[i]);
            int gx = (int)lroundf(position.x);
            int gy = HEIGHT - (int)lroundf(position.y);
            leds[XY(gx, gy)] = colors[i % (sizeof(colors) / sizeof(colors[0]))];
            if ((gx >= 0 && gx < WIDTH) && (gy >= 0 && gy < HEIGHT))
                marblesVisible = true;
        }

        // If no marbles are visible (all have fallen off the bottom), reset their positions
        if (!marblesVisible)
        {
            // make sure we can get the world mutex before resetting the world
            if (xSemaphoreTake(worldMutex, portMAX_DELAY))
            {
                // Move marbles to new random positions on the top row
                for (int i = 0; i < MARBLE_COUNT; ++i)
                {
                    float x = (float)(random(0, WIDTH));
                    b2Vec2 newPos = {x, (float)HEIGHT};
                    b2Body_SetTransform(marbles[i], newPos, b2MakeRot(0.0f)); // Move to new location

                    // Give each an initial push
                    float vx = (random(-100, 101)) / 50.0f; // ~[-2, 2] m/s
                    float vy = (random(10, 151)) / 50.0f;   // ~[0.2, 3] m/s upward
                    b2Body_SetLinearVelocity(marbles[i], (b2Vec2){vx, vy});
                    b2Body_SetAngularVelocity(marbles[i], 0.0f); // Stop spin
                }
                xSemaphoreGive(worldMutex);
            }
        }

        leds_dirty = true;
    }
}
