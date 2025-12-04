#include "main.h"
#include "debug.h"
#include "render.h"
#include "physics.h"
#include "bounce.h"

// Track our marbles so we can move them around
#define MARBLE_COUNT 8
static b2BodyId marbles[MARBLE_COUNT];

static void ResetMarbles()
{
    // Move marbles to new random positions above the visible area
    for (int i = 0; i < MARBLE_COUNT; ++i)
    {
        float x = (float)(random(0, WIDTH));
        float y = (float)(HEIGHT - random(1, HEIGHT / 4));
        b2Vec2 position = b2Vec2{x, y};
        b2Rot rotation = b2MakeRot(0.0f);
        b2Body_SetTransform(marbles[i], position, rotation); // Move to new location

        // Give each an initial push
        float vx = (random(-100, 101)) / 50.0f; // ~[-2, 2] m/s
        float vy = (random(10, 151)) / 50.0f;   // ~[0.2, 3] m/s upward
        b2Body_SetLinearVelocity(marbles[i], (b2Vec2){vx, vy});
        b2Body_SetAngularVelocity(marbles[i], 0.0f); // Stop spin
    }
}

static void setupWorld()
{
    // create the world
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, -9.8f};
    world = b2CreateWorld(&worldDef);

    CreateWall((float)WIDTH / 2.0f, -0.125f, (float)WIDTH + 0.5f, 0.25f);                       // floor
    CreateWall(-0.25f, (float)HEIGHT / 2.0f, 0.25f, (float)HEIGHT + 2.0f);                      // left wall
    CreateWall((float)WIDTH - 1.0f + 0.25f, (float)HEIGHT / 2.0f, 0.25f, (float)HEIGHT + 2.0f); // right wall

    // Spawn marbles at random positions above the visible area
    for (int i = 0; i < MARBLE_COUNT; ++i)
    {
        marbles[i] = CreateCircle(0.0f, 0.0f, 0.5f);
    }
}

void bounce_enter()
{
    // Initializie physics world and start physics task
    DB_PRINTLN("Entering Bounce mode");
    setupWorld();
    physics_enter();

    // Small delay to ensure physics task is running
    vTaskDelay(pdMS_TO_TICKS(100));

    // position the marbles (after physics task is running and world is valid)
    if (xSemaphoreTake(worldMutex, portMAX_DELAY))
    {
        ResetMarbles();
        xSemaphoreGive(worldMutex);
    }
}

void bounce_leave()
{
    physics_leave();
    DB_PRINTLN("Leaving Bounce mode");
}

void bounce_loop()
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

        // Draw marbles at their current positions
        FastLED.clear();
        for (int i = 0; i < MARBLE_COUNT; ++i)
        {
            if (!b2Body_IsValid(marbles[i]))
                continue;

            b2Vec2 position = b2Body_GetPosition(marbles[i]);
            int gx = (int)lroundf(position.x);
            int gy = HEIGHT - (int)lroundf(position.y);
            leds[XY(gx, gy)] = colors[i % (sizeof(colors) / sizeof(colors[0]))];
        }

        leds_dirty = true;
    }

    // reset marble positions every 10 seconds for demo purposes
    EVERY_N_SECONDS(10)
    {
        //        DB_PRINTF("Free heap: %u bytes | Largest block: %u bytes\r\n", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        //        DB_PRINTF("Physics stack high watermark: %d bytes\r\n", uxTaskGetStackHighWaterMark(physicsTaskHandle));

        // make sure we can get the world mutex before resetting the world
        if (xSemaphoreTake(worldMutex, portMAX_DELAY))
        {
            ResetMarbles();
            xSemaphoreGive(worldMutex);
        }
    }
}
