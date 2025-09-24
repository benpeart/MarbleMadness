#include "main.h"
#include "debug.h"
#include "physics.h"

// ID of the Box2D world instance
b2WorldId world = B2_NULL_ID;

// ----- Physics world -----
TaskHandle_t physicsTaskHandle = NULL;
SemaphoreHandle_t worldMutex = xSemaphoreCreateMutex();

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
        .friction = 0.0f,
        .restitution = 0.0f};

    // Attach the shape to the body
    b2CreatePolygonShape(body, &shapeDef, &box);
    return body;
}

b2BodyId CreateCircle(float x, float y, float r, float friction, float restitution, b2BodyType type)
{
    // Create the body
    b2BodyDef def = b2DefaultBodyDef();
    def.type = type;
    def.position = (b2Vec2){x, y};
    b2BodyId body = b2CreateBody(world, &def);

    // create the circle shape
    b2Circle circle = {0};
    circle.radius = r;

    // Create the shape definition
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 5.5f;
    shapeDef.material = (b2SurfaceMaterial){
        .friction = friction,
        .restitution = restitution};

    // Attach the shape to the body
    b2CreateCircleShape(body, &shapeDef, &circle);
    return body;
}

// ----- Physics task (Core 0) -----
void physicsTask(void *pvParameters)
{
    const TickType_t delay = pdMS_TO_TICKS(1000 / 60); // 60Hz
    while (true)
    {
        // wait until we get the world mutex
        if (xSemaphoreTake(worldMutex, portMAX_DELAY))
        {
            // then step the world and release the mutex
            b2World_Step(world, 1.0f / 60.0f, 1);
            xSemaphoreGive(worldMutex);
            vTaskDelay(delay);
        }
    }
}

void physics_enter()
{
    // Initializie physics world and start physics task
    DB_PRINTLN("Creating physics task");
    xTaskCreatePinnedToCore(physicsTask, "physicsTask", 32768, NULL, 1, &physicsTaskHandle, 0);
}

void physics_leave()
{
    if (physicsTaskHandle)
    {
        if (xSemaphoreTake(worldMutex, portMAX_DELAY))
        {
            vTaskDelete(physicsTaskHandle);
            physicsTaskHandle = NULL;
            if (!B2_IS_NULL(world))
                b2DestroyWorld(world);
            world = B2_NULL_ID;
            xSemaphoreGive(worldMutex);
        }
    }
    DB_PRINTLN("Leaving physics task");
}
