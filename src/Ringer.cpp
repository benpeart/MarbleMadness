#include "main.h"
#include "debug.h"
#include "render.h"
#include "physics.h"
#include "Ringer.h"

// Track our marbles so we can move them around
#define MARBLE_COUNT 8
static b2BodyId marbles[MARBLE_COUNT];

static void ResetMarbles()
{
    b2Vec2 position;

    // move the shooter marble to a random perimeter location and aim it toward the center
    float px, py;
    int edge = random(0, 3); // 0=top, 1=right, 2=bottom, 3=left
    switch (edge)
    {
    case 0: // top edge
        px = (float)random(0, WIDTH - 1);
        py = (float)(HEIGHT - 1);
        break;
    case 1: // right edge
        px = (float)(WIDTH - 1);
        py = (float)random(0, HEIGHT - 1);
        break;
    case 2: // bottom edge
        px = (float)random(0, WIDTH - 1);
        py = 0.0f;
        break;
    case 3: // left edge
        px = 0.0f;
        py = (float)random(0, HEIGHT - 1);
        break;
    }
    position = {px, py};
    b2Body_SetTransform(marbles[0], position, b2MakeRot(0.0f)); // Move to new location

    // Compute direction vector from spawn point to center
    float cx = (float)(WIDTH / 2);
    float cy = (float)(HEIGHT / 2);
    float dx = cx - px;
    float dy = cy - py;
    float dist = sqrtf(dx * dx + dy * dy);

    // Normalize and scale to desired speed
    float speed = random(10, 20);
    if (dist > 1e-6f)
    {
        dx = (dx / dist) * speed;
        dy = (dy / dist) * speed;
    }
    else
    {
        dx = 0.0f;
        dy = speed;
    }
    b2Body_SetLinearVelocity(marbles[0], (b2Vec2){dx, dy});

    // move marbles to random positions around the center
    for (int i = 1; i < MARBLE_COUNT; ++i)
    {
        position = {(float)(random(WIDTH * 1 / 3, WIDTH * 2 / 3)), (float)(random(HEIGHT * 1 / 3, HEIGHT * 2 / 3))};
        b2Body_SetTransform(marbles[i], position, b2MakeRot(0.0f)); // Move to new location
        b2Body_SetLinearVelocity(marbles[i], (b2Vec2){0, 0});
        b2Body_SetAngularVelocity(marbles[i], 0.0f); // Stop spin
    }
}

// ----- Physics setup -----
static void setupWorld()
{
    // create the world
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    world = b2CreateWorld(&worldDef);

#if 0
    // make a box the marbles can bounce around in
    CreateWall((float)WIDTH / 2.0f, HEIGHT+0.125f, (float)WIDTH + 0.5f, 0.25f);                 // ceiling
    CreateWall(-0.25f, (float)HEIGHT / 2.0f, 0.25f, (float)HEIGHT + 2.0f);                      // left wall
    CreateWall((float)WIDTH / 2.0f, -0.125f, (float)WIDTH + 0.5f, 0.25f);                       // floor
    CreateWall((float)WIDTH - 1.0f + 0.25f, (float)HEIGHT / 2.0f, 0.25f, (float)HEIGHT + 2.0f); // right wall
#endif
    // spawn the marbles
    marbles[0] = CreateCircle(0, 0, 0.9f, 0.3f, 0.85f);
    for (int i = 1; i < MARBLE_COUNT; ++i)
        marbles[i] = CreateCircle(0, 0, 0.45f, 0.3f, 0.85f);

    // now move the marbles into position
    ResetMarbles();
}

void ringer_enter()
{
    // Initializie physics world and start physics task
    DB_PRINTLN("Entering ringer mode");
    setupWorld();
    physics_enter();
}

void ringer_leave()
{
    physics_leave();
    DB_PRINTLN("Leaving ringer mode");
}

void ringer_loop()
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
    EVERY_N_SECONDS(5)
    {
        //        DB_PRINTF("Free heap: %u bytes | Largest block: %u bytes\r\n", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        //        DB_PRINTF("Physics stack high watermark: %d bytes\r\n", uxTaskGetStackHighWaterMark(physicsTaskHandle));

        // make sure we can get the world mutex before resetting the world
        if (xSemaphoreTake(worldMutex, portMAX_DELAY))
        {
            // Move marbles to new random positions above the visible area
            ResetMarbles();
            xSemaphoreGive(worldMutex);
        }
    }
}
