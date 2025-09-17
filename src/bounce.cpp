#include "main.h"
#include "settings.h"
#include "render.h"
#include "bounce.h"
#include "debug.h"

// ----- Box2D (full version) -----
// You have to edit constants.h and set B2_MAX_WORLDS to 1 for it to run on an ESP32
#include <box2d/box2d.h>

#ifdef CONNECT_FOUR
#define MARBLE_COUNT 19
#else
#define MARBLE_COUNT 8
#endif // CONNECT_FOUR

// ----- Physics world -----
TaskHandle_t physicsTaskHandle = NULL;
SemaphoreHandle_t worldMutex = xSemaphoreCreateMutex();

// Create Box2D world with gravity
b2WorldId world;
b2BodyId marbles[MARBLE_COUNT];

b2BodyId CreateWall(float x, float y, float w, float h)
{
    // Create the body
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = (b2Vec2){x, y}; // Center of the wall
    b2BodyId body = b2CreateBody(world, &bodyDef);

    // Use b2MakeBox to generate the polygon
    b2Polygon box = b2MakeBox(w * 0.5f, h * 0.5f); // half extents

    // Create the shape definition
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material = (b2SurfaceMaterial){
        .friction = 0.8f,
        .restitution = 0.0f};

    // Attach the shape to the body
    b2CreatePolygonShape(body, &shapeDef, &box);
    return body;
}

b2BodyId SpawnMarble(float x, float y, float r)
{
    // Create the body
    b2BodyDef def = b2DefaultBodyDef();
    def.type = b2_dynamicBody;
    def.position = (b2Vec2){x, y};
    b2BodyId body = b2CreateBody(world, &def);

    // create the circle shape
    b2Circle circle = {0};
    circle.radius = r;

    // Create the shape definition
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 5.5f;
    shapeDef.material = (b2SurfaceMaterial){
        .friction = 0.3f,
        .restitution = 0.85f}; // The coefficient of restitution (CoR) for a glass marble typically falls in the range of 0.84 to 0.86.

    // Attach the shape to the body
    b2CreateCircleShape(body, &shapeDef, &circle);
    return body;
}

// ----- Physics setup -----
void setupWorld()
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
#ifdef CONNECT_FOUR
        marbles[i] = SpawnMarble(i, HEIGHT - 1, 0.45f);
#else
        float x = (float)(random(0, WIDTH));
        float y = (float)(HEIGHT - random(1, HEIGHT / 4));
        marbles[i] = SpawnMarble(x, HEIGHT - 1, 0.45f);

        // Give each an initial push
        float vx = (random(-100, 101)) / 50.0f; // ~[-2, 2] m/s
        float vy = (random(10, 151)) / 50.0f;   // ~[0.2, 3] m/s upward
        b2Body_SetLinearVelocity(marbles[i], (b2Vec2){vx, vy});
#endif // CONNECT_FOUR
    }
}

// ----- Physics task (Core 0) -----
void physicsTask(void *pvParameters)
{
    const TickType_t delay = pdMS_TO_TICKS(1000 / 60); // 60Hz
    while (true)
    {
        // wait until we get the world mutex
        if (xSemaphoreTake(worldMutex, 0))
        {
            // then step the world and release the mutex
            if (B2_IS_NON_NULL(world))
                b2World_Step(world, 1.0f / 60.0f, 1);
            xSemaphoreGive(worldMutex);
            vTaskDelay(delay);
        }
    }
}

void bounce_enter()
{
    // Initializie physics world and start physics task
    DB_PRINTLN("Entering Bounce mode");
    setupWorld();
    xTaskCreatePinnedToCore(physicsTask, "physicsTask", 32768, NULL, 1, &physicsTaskHandle, 0);
}

void bounce_leave()
{
    if (physicsTaskHandle)
    {
        if (xSemaphoreTake(worldMutex, portMAX_DELAY))
        {
            vTaskDelete(physicsTaskHandle);
            physicsTaskHandle = NULL;
            if (!B2_IS_NULL(world))
                b2DestroyWorld(world);
            world = b2_nullWorldId;
        }
        xSemaphoreGive(worldMutex);
    }
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
        //        DB_PRINTF("Free heap: %u bytes | Largest block: %u bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        //        DB_PRINTF("Physics stack high watermark: %d bytes\n", uxTaskGetStackHighWaterMark(physicsTaskHandle));

        // make sure we can get the world mutex before resetting the world
        if (xSemaphoreTake(worldMutex, portMAX_DELAY))
        {
#ifdef CONNECT_FOUR
            // Move marbles into new positions along top row
            for (int i = 0; i < MARBLE_COUNT; ++i)
            {
                b2Body_SetTransform(marbles[i], (b2Vec2){(float)i, (float)(HEIGHT - 1)}, b2MakeRot(0.0f)); // Move to new location
                b2Body_SetLinearVelocity(marbles[i], (b2Vec2){0, 0});
                b2Body_SetAngularVelocity(marbles[i], 0.0f); // Stop spin
            }
#else
            // Move marbles to new random positions above the visible area
            for (int i = 0; i < MARBLE_COUNT; ++i)
            {
                float x = (float)(random(0, WIDTH));
                float y = (float)(HEIGHT - random(1, HEIGHT / 4));
                b2Vec2 newPos = {x, y};
                b2Body_SetTransform(marbles[i], newPos, b2MakeRot(0.0f)); // Move to new location

                // Give each an initial push
                float vx = (random(-100, 101)) / 50.0f; // ~[-2, 2] m/s
                float vy = (random(10, 151)) / 50.0f;   // ~[0.2, 3] m/s upward
                b2Body_SetLinearVelocity(marbles[i], (b2Vec2){vx, vy});
                b2Body_SetAngularVelocity(marbles[i], 0.0f); // Stop spin
            }
#endif // CONNECT_FOUR
            xSemaphoreGive(worldMutex);
        }
    }
}
