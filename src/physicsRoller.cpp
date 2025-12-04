#include "main.h"
#include "debug.h"
#include "render.h"
#include "physics.h"
#include "physicsRoller.h"

// Track our marbles so we can move them around
#define MARBLE_COUNT 7
#define TRACK_COUNT 8
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
    b2BodyId track;
    int x = 0;

    // world coords are origin bottom-left, so y increases upward
    tracks[x++] = CreateLine(-1, 0, -1, HEIGHT - 1);        // left wall
    tracks[x++] = CreateLine(WIDTH, 0, WIDTH, HEIGHT - 1);  // right wall

    tracks[x++] = CreateLine(-1, 17, WIDTH - 2, 16);
    tracks[x++] = CreateLine(1, 13, WIDTH - 0, 14);
    tracks[x++] = CreateLine(-1, 11, WIDTH - 2, 10);
    tracks[x++] = CreateLine(1,  7, WIDTH - 0,  8);
    tracks[x++] = CreateLine(-1,  5, WIDTH - 2,  4);
    tracks[x++] = CreateLine(1,  1, WIDTH - 0,  2);
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

        // Draw tracks by iterating each body's shapes and rasterizing polygons
        for (int i = 0; i < TRACK_COUNT; ++i)
        {
            if (!b2Body_IsValid(tracks[i]))
                continue;

            // Get the body's transform for converting local vertices to world space
            b2Transform xf = b2Body_GetTransform(tracks[i]);

            // Get shapes associated with this body
            int shapeCount = b2Body_GetShapeCount(tracks[i]);
            if (shapeCount <= 0)
                continue;

            // Acquire shape ids into a stack buffer when small, otherwise allocate
            const int STACK_CAP = 8;
            b2ShapeId shapeIdsStack[STACK_CAP];
            b2ShapeId *shapeIds = shapeIdsStack;
            b2ShapeId *heapBuf = NULL;
            if (shapeCount > STACK_CAP)
            {
                heapBuf = (b2ShapeId *)malloc(sizeof(b2ShapeId) * shapeCount);
                if (heapBuf == NULL)
                    continue; // allocation failed
                shapeIds = heapBuf;
            }

            int got = b2Body_GetShapes(tracks[i], shapeIds, shapeCount);
            for (int s = 0; s < got; ++s)
            {
                b2ShapeId shapeId = shapeIds[s];
                b2ShapeType type = b2Shape_GetType(shapeId);

                if (type != b2_polygonShape)
                    continue;

                // Get polygon geometry
                b2Polygon poly = b2Shape_GetPolygon(shapeId);
                int vcount = poly.count;
                if (vcount <= 0)
                    continue;

                // Transform vertices to world space
                // Use a small stack array since polygons are small
                b2Vec2 worldVerts[B2_MAX_POLYGON_VERTICES];
                for (int v = 0; v < vcount; ++v)
                {
                    worldVerts[v] = b2TransformPoint(xf, poly.vertices[v]);
                }

                // Compute integer bounding box in grid coordinates
                int minGX = NUM_COLS, maxGX = 0, minGY = NUM_ROWS, maxGY = 0;
                for (int v = 0; v < vcount; ++v)
                {
                    int gx = (int)lroundf(worldVerts[v].x);
                    int gy = HEIGHT - (int)lroundf(worldVerts[v].y);

                    if (gx < minGX)
                        minGX = gx;
                    if (gx > maxGX)
                        maxGX = gx;
                    if (gy < minGY)
                        minGY = gy;
                    if (gy > maxGY)
                        maxGY = gy;
                }

                // Clip to LED grid
                if (minGX < 0)
                    minGX = 0;
                if (minGY < 0)
                    minGY = 0;
                if (maxGX >= NUM_COLS)
                    maxGX = NUM_COLS - 1;
                if (maxGY >= NUM_ROWS)
                    maxGY = NUM_ROWS - 1;
                if (minGX > maxGX || minGY > maxGY)
                    continue;

                // Rasterize: for each LED cell in bbox, test for any intersection between the polygon and the cell
                for (int gy = minGY; gy <= maxGY; ++gy)
                {
                    for (int gx = minGX; gx <= maxGX; ++gx)
                    {
                        // Define cell rectangle in world-space coordinates: [rx1, rx2) x [ry1, ry2)
                        float rx1 = (float)gx;
                        float rx2 = (float)(gx + 1);
                        // note: grid y -> world y mapping: worldY = HEIGHT - gy
                        float ry2 = (float)(HEIGHT - gy);
                        float ry1 = (float)(HEIGHT - (gy + 1));

                        // Helper: point in polygon (ray crossing) using transformed vertices
                        auto pointInPoly = [&](float px, float py) -> bool
                        {
                            bool inside = false;
                            for (int a = 0, b = vcount - 1; a < vcount; b = a++)
                            {
                                float ay = worldVerts[a].y;
                                float by = worldVerts[b].y;
                                float ax = worldVerts[a].x;
                                float bx = worldVerts[b].x;

                                bool intersect = ((ay > py) != (by > py)) && (px < (bx - ax) * (py - ay) / (by - ay + 1e-12f) + ax);
                                if (intersect)
                                    inside = !inside;
                            }
                            return inside;
                        };

                        // Helper: point inside rect
                        auto pointInRect = [&](float px, float py) -> bool
                        {
                            return px >= rx1 && px <= rx2 && py >= ry1 && py <= ry2;
                        };

                        // Helper: segment-segment intersection
                        auto segIntersect = [&](float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) -> bool
                        {
                            auto orient = [](float ax, float ay, float bx, float by, float cx, float cy) -> float
                            {
                                return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
                            };

                            float o1 = orient(x1, y1, x2, y2, x3, y3);
                            float o2 = orient(x1, y1, x2, y2, x4, y4);
                            float o3 = orient(x3, y3, x4, y4, x1, y1);
                            float o4 = orient(x3, y3, x4, y4, x2, y2);

                            if (((o1 > 0 && o2 < 0) || (o1 < 0 && o2 > 0)) && ((o3 > 0 && o4 < 0) || (o3 < 0 && o4 > 0)))
                                return true;

                            // Collinear / touching cases: check bounding boxes
                            auto onSeg = [&](float ax, float ay, float bx, float by, float cx, float cy) -> bool
                            {
                                if (fminf(ax, bx) - 1e-6f <= cx && cx <= fmaxf(ax, bx) + 1e-6f && fminf(ay, by) - 1e-6f <= cy && cy <= fmaxf(ay, by) + 1e-6f)
                                    return true;
                                return false;
                            };

                            if (fabsf(o1) < 1e-6f && onSeg(x1, y1, x2, y2, x3, y3))
                                return true;
                            if (fabsf(o2) < 1e-6f && onSeg(x1, y1, x2, y2, x4, y4))
                                return true;
                            if (fabsf(o3) < 1e-6f && onSeg(x3, y3, x4, y4, x1, y1))
                                return true;
                            if (fabsf(o4) < 1e-6f && onSeg(x3, y3, x4, y4, x2, y2))
                                return true;

                            return false;
                        };

                        // Check 1: if any polygon vertex is inside the rect
                        bool intersects = false;
                        for (int pv = 0; pv < vcount; ++pv)
                        {
                            float vx = worldVerts[pv].x;
                            float vy = worldVerts[pv].y;
                            if (pointInRect(vx, vy))
                            {
                                intersects = true;
                                break;
                            }
                        }

                        // Check 2: if any rect corner is inside polygon
                        if (!intersects)
                        {
                            float cx[4] = {rx1, rx2, rx2, rx1};
                            float cy[4] = {ry1, ry1, ry2, ry2};
                            for (int c = 0; c < 4; ++c)
                            {
                                if (pointInPoly(cx[c], cy[c]))
                                {
                                    intersects = true;
                                    break;
                                }
                            }
                        }

                        // Check 3: if any polygon edge intersects any rect edge
                        if (!intersects)
                        {
                            // rect edges
                            float rx[4] = {rx1, rx2, rx2, rx1};
                            float ry_[4] = {ry1, ry1, ry2, ry2};

                            for (int e = 0; e < vcount && !intersects; ++e)
                            {
                                int en = (e + 1) % vcount;
                                float x1 = worldVerts[e].x;
                                float y1 = worldVerts[e].y;
                                float x2 = worldVerts[en].x;
                                float y2 = worldVerts[en].y;

                                for (int re = 0; re < 4; ++re)
                                {
                                    int rn = (re + 1) % 4;
                                    float x3 = rx[re];
                                    float y3 = ry_[re];
                                    float x4 = rx[rn];
                                    float y4 = ry_[rn];

                                    if (segIntersect(x1, y1, x2, y2, x3, y3, x4, y4))
                                    {
                                        intersects = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if (intersects)
                        {
                            leds[XY(gx, gy)] = CRGB::DarkSlateGray;
                        }
                    }
                }
            }

            if (heapBuf)
            {
                free(heapBuf);
            }
        }

        // Draw marbles at their current positions
        for (int i = 0; i < marbleCount; i++)
        {
            if (!b2Body_IsValid(marbles[i]))
                continue;

            b2Vec2 position = b2Body_GetPosition(marbles[i]);
            int gx = (int)lroundf(position.x);
            int gy = HEIGHT - (int)ceilf(position.y);

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
                marbles[marbleCount++] = CreateCircle(0, HEIGHT - 1, 0.5f, 0.0f, 0.85f);
                xSemaphoreGive(worldMutex);
            }
        }
    }
}
