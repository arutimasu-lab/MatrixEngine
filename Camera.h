// Camera.h
#pragma once
#include <GL/glut.h>
#include <cmath>
#include "Map.h"

class Camera {
public:
    Camera();

    void setPosition(float x, float y, float z);
    void setRotation(float yaw, float pitch);

    void moveForward(float delta);
    void moveRight(float delta);
    void rotate(float dYaw, float dPitch);

    void apply() const;


    float getX() const { return x; }
    float getZ() const { return z; }
    void tryMove(float dx, float dz, const Level& map, float radius);

    void getForwardDelta(float distance, float& dx, float& dz) const;

    void getRightDelta(float distance, float& dx, float& dz) const;

private:
    float x, y, z;
    float yaw;   // поворот влево-вправо (в градусах)
    float pitch; // поворот вверх-вниз (в градусах)

    static constexpr float MAX_PITCH = 89.0f;
};