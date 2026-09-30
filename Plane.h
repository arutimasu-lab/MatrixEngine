// Plane.h
#pragma once
#include <GL/glut.h>
#include <string>
#include "Texture.h"

class Plane {
public:
    // facingUp = true для пола (нормаль вверх), false для потолка (нормаль вниз)
    Plane(bool facingUp, float x, float y, float z, float size, const std::string& texturePath);
    void render();

private:
    bool facingUp;
    float x, y, z;
    float size;
    std::string texturePath;
    Texture* texture = nullptr;
};
