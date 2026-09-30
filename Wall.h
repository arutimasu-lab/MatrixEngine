// Wall.h
#pragma once
#include <GL/glut.h>
#include <string>
#include "Texture.h"

enum class WallType {
    Horizontal,   // грань в плоскости XY (стена вдоль Z)
    Vertical,     // грань в плоскости ZY (стена вдоль X)
    Full          // 4 грани — сплошной блок
};

class Wall {
public:
    Wall(WallType type, float x, float y, float z, float size, const std::string& texturePath);
    void render();

private:
    WallType type;
    float x, y, z;
    float size;
    std::string texturePath;
    Texture* texture = nullptr;

    void renderFull();
    void renderHorizontal();
    void renderVertical();
};