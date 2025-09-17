#include "main.h"
#include "settings.h"
#include "render.h"
#include "pachinko.h"
#include "debug.h"

// ----- Box2D (full version) -----
// You have to edit constants.h and set B2_MAX_WORLDS to 1 for it to run on an ESP32
#include <box2d/box2d.h>

// ----- Physics world -----
static TaskHandle_t physicsTaskHandle = NULL;
static SemaphoreHandle_t worldMutex = xSemaphoreCreateMutex();

// Create Box2D world with gravity
#define MARBLE_COUNT 6
static b2WorldId world;
static b2BodyId marbles[MARBLE_COUNT];

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

static b2BodyId CreateWall(float x, float y, float w, float h)
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

static b2BodyId CreateMarble(float x, float y, float r)
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

static b2BodyId CreatePin(float x, float y, float r)
{
    // Create the body
    b2BodyDef def = b2DefaultBodyDef();
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
        .restitution = 0.80f}; // The coefficient of restitution (CoR) for a steel pachinko pin typically falls in the range of 0.80 to 0.85.

    // Attach the shape to the body
    b2CreateCircleShape(body, &shapeDef, &circle);
    return body;
}

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
                CreatePin((float)x, (float)(HEIGHT - y), 0.25f);
            }
        }
    }

    // Spawn marbles at random positions along the top row
    for (int i = 0; i < MARBLE_COUNT; ++i)
    {
        float x = (float)(random(0, WIDTH));
        marbles[i] = CreateMarble(x, (float)HEIGHT, 0.45f);

        // Give each an initial push
        float vx = (random(-100, 101)) / 50.0f; // ~[-2, 2] m/s
        float vy = (random(10, 151)) / 50.0f;   // ~[0.2, 3] m/s upward
        b2Body_SetLinearVelocity(marbles[i], (b2Vec2){vx, vy});
    }
}

// ----- Physics task (Core 0) -----
static void physicsTask(void *pvParameters)
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

void pachinko_enter()
{
    // Initialize physics world and start physics task
    DB_PRINTLN("Entering Pachinko mode");
    setupWorld();
    xTaskCreatePinnedToCore(physicsTask, "physicsTask", 32768, NULL, 1, &physicsTaskHandle, 0);
}

void pachinko_leave()
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
