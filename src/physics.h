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

void physics_enter();
void physics_leave();

#endif // PHYSICS_H
