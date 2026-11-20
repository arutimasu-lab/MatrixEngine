// Cube.h
#pragma once
#include <GL/glut.h>
#include <string>
#include "Texture.h"

class Cube {
public:
    Cube(float x, float y, float z,
        float sx = 1.0f, float sy = 1.0f, float sz = 1.0f,
        const std::string& texturePath = "");
    ~Cube();

    void render();
    void setTexture(const std::string& path);

private:
    float x, y, z;
    float sx, sy, sz;
    std::string texturePath;
    Texture* texture = nullptr;
};