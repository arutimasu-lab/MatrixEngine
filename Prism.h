#pragma once
// Prism.h
#pragma once
#include <GL/glut.h>
#include <string>
#include "Texture.h"

class Prism {
public:
    // size — ширина/высота призмы
    // depth — глубина (толщина)
    Prism(float x, float y, float z,
        float size, float depth,
        const std::string& texturePath);
    ~Prism();

    void render();

    // Для смены текстуры (например, цифры на шкафу)
    void setTexture(const std::string& texturePath);

private:
    float x, y, z;
    float size;
    float depth;
    std::string texturePath;
    Texture* texture = nullptr;

    void renderFace(float v0x, float v0y, float v0z,
        float v1x, float v1y, float v1z,
        float v2x, float v2y, float v2z,
        float v3x, float v3y, float v3z,
        float nx, float ny, float nz);
};