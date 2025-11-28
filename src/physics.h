#ifndef PHYSICS_H
#define PHYSICS_H

#include <Arduino.h>

// ----- Box2D (full version) -----
// You have to edit constants.h and set B2_MAX_WORLDS to 1 for it to run on an ESP32
#include <box2d/box2d.h>

// Before modifying anything in the physics world, you must take the mutex
// and release it when done
extern b2WorldId world;
extern SemaphoreHandle_t worldMutex;

// helper functions
b2BodyId CreateWall(float x, float y, float w, float h);
b2BodyId CreateCircle(float x, float y, float r, float friction = 0.3f, float restitution = 0.85f, b2BodyType type = b2_dynamicBody);

// Create a thin box (polygon) between two points (world coords: bottom-left is 0,0)
b2BodyId CreateLine(float x1, float y1, float x2, float y2, float thickness = 0.9f, float friction = 0.0f, float restitution = 0.0f);

void physics_enter();
void physics_leave();

#endif // PHYSICS_H
