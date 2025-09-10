#include "main.h"
#include "settings.h"
#include "render.h"
#include "bounce.h"
#include "debug.h"

// ----- Box2D-Lite (original) -----
#include <box2d-lite/World.h>
#include <box2d-lite/Body.h>

#define DEFAULT_MILLIS 100
#define MIN_MILLIS 0
#define MAX_MILLIS (4 * DEFAULT_MILLIS)

// ----- Physics world -----
TaskHandle_t physicsTaskHandle = NULL;

// Supply gravity vector and solver iterations directly to constructor
World world(Vec2(0.0f, -9.8f), 10); // 10 iterations is a common default

// Marbles (dynamic small boxes)
static const int NUM_MARBLES = 9;
Body marbles[NUM_MARBLES];

// Boundaries (static)
Body ground, ceiling, wallLeft, wallRight;

// Shared positions for LED task (atomic ints are enough here)
volatile int marbleX[NUM_MARBLES];
volatile int marbleY[NUM_MARBLES];

// Physics-to-LED scale
// 1 physics unit = 1 LED cell
#define SCALE 1.0f

// Create a body with given center position, half-extents, and mass
void makeBody(Body &b, float cx, float cy, float hx, float hy, float mass, bool isStatic = false)
{
    // Call Set with half-extents and mass
    b.Set(Vec2(hx, hy), mass);

    // Set initial position
    b.position.Set(cx, cy);

    // Make static if requested
    if (isStatic)
    {
        b.invMass = 0.0f;
        b.invI = 0.0f;
    }

    // Add to the world so it will be simulated
    world.Add(&b);
}

// ----- Physics setup -----
void setupWorld()
{
    // Walls: define a 19x19 interior with thin static boxes as boundaries
    // Ground at y = -0.5 (thin slab)
    makeBody(ground, NUM_COLS / 2.0f, -0.1f, NUM_COLS / 2.0f, 0.1f, 0.0f, true);
    ground.restitution = 1.15f; // make the ground bouncy
    // Ceiling at y = NUM_ROWS + 0.5
    makeBody(ceiling, NUM_COLS / 2.0f, NUM_ROWS + 0.1f, NUM_COLS / 2.0f, 0.1f, 0.0f, true);
    // Left wall at x = -0.5
    makeBody(wallLeft, -0.1f, NUM_ROWS / 2.0f, 0.1f, NUM_ROWS / 2.0f, 0.0f, true);
    // Right wall at x = NUM_COLS + 0.5
    makeBody(wallRight, NUM_COLS + 0.1f, NUM_ROWS / 2.0f, 0.1f, NUM_ROWS / 2.0f, 0.0f, true);

    // Marbles: small dynamic boxes (half-extents 0.5 ~ 1 LED)
    // Random starting positions and velocities
    for (int i = 0; i < NUM_MARBLES; i++)
    {
        float startX = random(2, NUM_COLS - 2);
        float startY = random(NUM_ROWS / 2, NUM_ROWS - 3);
        makeBody(marbles[i], startX, startY, 0.5f, 0.5f, 5.35f, false); // mass is 5.35g for 16mm glass marble
        marbles[i].restitution = 1.15f;                                 // The coefficient of restitution (CoR) for a glass marble typically falls in the range of 0.84 to 0.86.

        // Give each an initial push
        float vx = (random(-100, 101)) / 50.0f; // ~[-2, 2] m/s
        float vy = (random(10, 151)) / 50.0f;   // ~[0.2, 3] m/s upward
        marbles[i].velocity.Set(vx, vy);
    }
}

// ----- Physics task (Core 0) -----
void physicsTask(void *pv)
{
    const float timeStep = 1.0f / 60.0f;
    const TickType_t delayTicks = pdMS_TO_TICKS(1000 / 60);

    for (;;)
    {
        world.Step(timeStep);

        // Share marble positions with LED task
        for (int i = 0; i < NUM_MARBLES; i++)
        {
            int mx = (int)roundf(marbles[i].position.x * SCALE);
            int my = (int)roundf(marbles[i].position.y) * SCALE;
            marbleX[i] = constrain(mx, 0, NUM_COLS - 1);
            marbleY[i] = constrain(my, 0, NUM_ROWS - 1);
        }

        vTaskDelay(delayTicks);
    }
}

void bounce_enter()
{
    DB_PRINTLN("Entering Bounce mode");

    // Reset physics world
    world.Clear();
    setupWorld();
    xTaskCreatePinnedToCore(physicsTask, "Physics", 2048, NULL, 1, &physicsTaskHandle, 0);
}

void bounce_leave()
{
    if (physicsTaskHandle)
        vTaskDelete(physicsTaskHandle);
    physicsTaskHandle = NULL;
    DB_PRINTLN("Leaving Bounce mode");
}

void bounce_loop()
{
    static byte speed = 92;
    static uint32_t t;

    EVERY_N_MILLIS_I(timer, DEFAULT_MILLIS)
    {
        timer.setPeriod(MAX_MILLIS - map(settings.speed, MIN_SPEED, MAX_SPEED, MIN_MILLIS, MAX_MILLIS));
        t += speed;

        CRGB colors[] = {
            CRGB::Red, CRGB::Green, CRGB::Blue,
            CRGB::Yellow, CRGB::Magenta, CRGB::Cyan};

        fill_solid(leds, NUM_STRIPS * NUM_LEDS_PER_STRIP, CRGB::Black);
        for (int i = 0; i < NUM_MARBLES; i++)
        {
            leds[XY(marbleX[i], NUM_ROWS - marbleY[i] - 1)] = colors[i % 6];
        }

        leds_dirty = true;
    }

    // reset world every 7 seconds for demo purposes
    EVERY_N_SECONDS(7)
    {
        DB_PRINTLN("Resetting physics world");
        world.Clear();
        setupWorld();
    }
}
