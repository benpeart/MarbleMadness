#include "main.h"
#include "debug.h"
#include "render.h"
#include "physics.h"
#include "physicsRoller.h"

// Track our marbles so we can move them around
#define MARBLE_COUNT 7
#define TRACK_COUNT 6
static b2BodyId marbles[MARBLE_COUNT];
static b2BodyId tracks[TRACK_COUNT];
static int marbleCount = 0;

static float MinimumRestitutionCallback(float restitutionA, uint64_t userMaterialIdA, float restitutionB, uint64_t userMaterialIdB)
{
    // Return the minimum restitution value so marbles bounce off each other but not the walls or track
    return fminf(restitutionA, restitutionB);
}

// ----- Physics setup -----
static void setupWorld()
{
    // create the world
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, -19.8f};
    world = b2CreateWorld(&worldDef);
    b2World_SetRestitutionCallback(world, MinimumRestitutionCallback);

    // create floor and walls
    //    CreateWall((float)WIDTH / 2.0f, -0.125f, (float)WIDTH + 0.5f, 0.25f);                       // floor
    CreateWall(-0.25f, (float)HEIGHT / 2.0f, 0.25f, (float)HEIGHT + 2.0f);                      // left wall
    CreateWall((float)WIDTH - 1.0f + 0.25f, (float)HEIGHT / 2.0f, 0.25f, (float)HEIGHT + 2.0f); // right wall

    // create some zig-zag tracks
    b2BodyId track;
    int x = 0;

    tracks[x++] = track = CreateWall((float)(WIDTH / 2.0f) - 1, (float)HEIGHT - 2.0f + 0.125f, (float)WIDTH - 1.5f, 0.25f);
    b2Body_SetTransform(track, b2Body_GetPosition(track), b2MakeRot(-5.5f * M_PI / 180.0f));

    tracks[x++] = track = CreateWall((float)(WIDTH / 2.0f) + 1, (float)HEIGHT - 5.0f + 0.125f, (float)WIDTH - 1.5f, 0.25f);
    b2Body_SetTransform(track, b2Body_GetPosition(track), b2MakeRot(5.5f * M_PI / 180.0f));

    tracks[x++] = track = CreateWall((float)(WIDTH / 2.0f) - 1, (float)HEIGHT - 8.0f + 0.125f, (float)WIDTH - 1.5f, 0.25f);
    b2Body_SetTransform(track, b2Body_GetPosition(track), b2MakeRot(-5.5f * M_PI / 180.0f));

    tracks[x++] = track = CreateWall((float)(WIDTH / 2.0f) + 1, (float)HEIGHT - 11.0f + 0.125f, (float)WIDTH - 1.5f, 0.25f);
    b2Body_SetTransform(track, b2Body_GetPosition(track), b2MakeRot(5.5f * M_PI / 180.0f));

    tracks[x++] = track = CreateWall((float)(WIDTH / 2.0f) - 1, (float)HEIGHT - 14.0f + 0.125f, (float)WIDTH - 1.5f, 0.25f);
    b2Body_SetTransform(track, b2Body_GetPosition(track), b2MakeRot(-5.5f * M_PI / 180.0f));

    tracks[x++] = track = CreateWall((float)(WIDTH / 2.0f) + 1, (float)HEIGHT - 17.0f + 0.125f, (float)WIDTH - 1.5f, 0.25f);
    b2Body_SetTransform(track, b2Body_GetPosition(track), b2MakeRot(5.5f * M_PI / 180.0f));
}

void physicsRoller_enter()
{
    // Initializie physics world and start physics task
    DB_PRINTLN("Entering physicsRoller mode");
    setupWorld();
    physics_enter();
}

void physicsRoller_leave()
{
    marbleCount = 0;
    physics_leave();
    DB_PRINTLN("Leaving physicsRoller mode");
}

void physicsRoller_loop()
{
    // ~60 FPS
    EVERY_N_MILLIS(16)
    {
        CRGB colors[] = {
            CRGB::Red,      // Bold and warm
            CRGB::Green,    // Natural and vibrant
            CRGB::Blue,     // Cool and deep
            CRGB::Purple,   // Rich and regal
            CRGB::Cyan,     // Tropical and fresh
            CRGB::Orange,   // Warm and punchy
            CRGB::Pink,     // Playful and vivid
            CRGB::Yellow,   // Bright and energetic
            CRGB::LimeGreen // Electric and sharp
        };

        FastLED.clear();

        // Draw tracks using b2Body_GetShapeCount and b2Body_GetShapes APIs
#if 0        
        for (int i = 0; i < TRACK_COUNT; ++i)
        {
            if (!b2Body_IsValid(tracks[i]))
                continue;

            b2Vec2 position = b2Body_GetPosition(tracks[i]);
            b2Transform transform = b2Body_GetTransform(tracks[i]);

            // Get the number of shapes associated with the body
            int shapeCount = b2Body_GetShapeCount(tracks[i]);

            // Iterate through each shape
            for (int shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex)
            {
                const b2Shape* shape = b2Body_GetShapes(tracks[i], shapeIndex);

                // Ensure the shape is a polygon
                if (shape->type != b2_polygonShape)
                    continue;

                const b2PolygonShape* polygon = (const b2PolygonShape*)shape;
                int32_t vertexCount = polygon->vertexCount;

                // Transform and map each vertex to the LED grid
                b2Vec2 vertices[vertexCount];
                for (int32_t j = 0; j < vertexCount; ++j)
                {
                    b2Vec2 vertex = polygon->vertices[j];
                    vertices[j].x = vertex.x * transform.q.c - vertex.y * transform.q.s + position.x;
                    vertices[j].y = vertex.x * transform.q.s + vertex.y * transform.q.c + position.y;
                }

                // Determine the bounding box of the polygon in LED grid space
                int minX = NUM_COLS, maxX = 0, minY = NUM_ROWS, maxY = 0;
                for (int32_t j = 0; j < vertexCount; ++j)
                {
                    int gx = (int)lroundf(vertices[j].x);
                    int gy = HEIGHT - (int)lroundf(vertices[j].y);

                    if (gx >= 0 && gx < NUM_COLS && gy >= 0 && gy < NUM_ROWS)
                    {
                        minX = fminf(minX, gx);
                        maxX = fmaxf(maxX, gx);
                        minY = fminf(minY, gy);
                        maxY = fmaxf(maxY, gy);
                    }
                }

                // Fill the polygon on the LED grid
                for (int gy = minY; gy <= maxY; ++gy)
                {
                    for (int gx = minX; gx <= maxX; ++gx)
                    {
                        leds[XY(gx, gy)] = CRGB::White; // Use white color for tracks
                    }
                }
            }
        }
#endif
        // Draw marbles at their current positions
        for (int i = 0; i < marbleCount; i++)
        {
            if (!b2Body_IsValid(marbles[i]))
                continue;

            b2Vec2 position = b2Body_GetPosition(marbles[i]);
            int gx = (int)lroundf(position.x);
            int gy = HEIGHT - (int)lroundf(position.y);

            // check if we need to reset to the top
            if (gy >= NUM_ROWS - 1)
            {
                gx = gy = 0;

                // make sure we can get the world mutex before resetting the marble
                if (xSemaphoreTake(worldMutex, portMAX_DELAY))
                {
                    // Move marble to the top row
                    b2Vec2 newPos = {0.0f, (float)HEIGHT - 1};
                    b2Body_SetTransform(marbles[i], newPos, b2MakeRot(0.0f)); // Move to new location

                    // reset it's velocity and spin
                    b2Body_SetLinearVelocity(marbles[i], (b2Vec2){0.0f, 0.0f});
                    b2Body_SetAngularVelocity(marbles[i], 0.0f); // Stop spin

                    xSemaphoreGive(worldMutex);
                }
            }

            // draw the marble at its current position
            leds[XY(gx, gy)] = colors[i % (sizeof(colors) / sizeof(colors[0]))];
            leds_dirty = true;
        }
    }

    // spawn more marbles every 5 seconds for demo purposes
    EVERY_N_SECONDS(5)
    {
        if (marbleCount < MARBLE_COUNT)
        {
            // make sure we can get the world mutex before adding a new marble
            if (xSemaphoreTake(worldMutex, portMAX_DELAY))
            {
                // Spawn a marble at the start of the track
                marbles[marbleCount++] = CreateCircle(0, HEIGHT - 1, 0.45f, 0.0f, 0.85f);
                xSemaphoreGive(worldMutex);
            }
        }
    }
}
