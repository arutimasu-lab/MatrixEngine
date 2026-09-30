// Camera.cpp
#include "Camera.h"

Camera::Camera()
    : x(0.0f), y(1.6f), z(0.0f), yaw(0.0f), pitch(0.0f) {
}

void Camera::setPosition(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

void Camera::setRotation(float yaw, float pitch) {
    this->yaw = yaw;
    this->pitch = pitch;
}

void Camera::moveForward(float delta) {
    // Движение по направлению взгляда, но без учёта pitch (чтобы не взлетать)
    float rad = yaw * 3.14159265f / 180.0f;
    x += -sinf(rad) * delta;
    z += -cosf(rad) * delta;
}

void Camera::moveRight(float delta) {
    float rad = yaw * 3.14159265f / 180.0f;
    x += cosf(rad) * delta;
    z += -sinf(rad) * delta;
}

void Camera::rotate(float dYaw, float dPitch) {
    yaw += dYaw;
    pitch += dPitch;
    if (pitch > MAX_PITCH)  pitch = MAX_PITCH;
    if (pitch < -MAX_PITCH) pitch = -MAX_PITCH;
}

void Camera::apply() const {
    // Направление взгляда вычисляем из yaw/pitch
    float radYaw = yaw * 3.14159265f / 180.0f;
    float radPitch = pitch * 3.14159265f / 180.0f;

    float dirX = -sinf(radYaw) * cosf(radPitch);
    float dirY = sinf(radPitch);
    float dirZ = -cosf(radYaw) * cosf(radPitch);

    gluLookAt(
        x, y, z,
        x + dirX, y + dirY, z + dirZ,
        0.0f, 1.0f, 0.0f
    );
}
// Camera.h (добавить метод)
void Camera::tryMove(float dx, float dz, const Level& map, float radius) {
    // Пробуем сдвинуться по X
    if (!map.collidesWithRadius(x + dx, z, radius)) {
        x += dx;
    }
    // Пробуем сдвинуться по Z
    if (!map.collidesWithRadius(x, z + dz, radius)) {
        z += dz;
    }
}
// Camera.h
void Camera::getForwardDelta(float distance, float& dx, float& dz) const {
    float rad = yaw * 3.14159265f / 180.0f;
    dx = -sinf(rad) * distance;
    dz = -cosf(rad) * distance;
}

void Camera::getRightDelta(float distance, float& dx, float& dz) const {
    float rad = yaw * 3.14159265f / 180.0f;
    dx = cosf(rad) * distance;
    dz = -sinf(rad) * distance;
}