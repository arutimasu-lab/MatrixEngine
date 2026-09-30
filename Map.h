// Map.h
#pragma once
#include <vector>
#include <string>
#include <map>

struct Level {
    std::map<char, std::string> objects;      // 'H' -> "wall_stone_h"
    std::vector<std::vector<char>> cells;     // матрица символов
    float gridSize = 2.0f;
    int spawnI = -1;
    int spawnJ = -1;

    bool isSolid(int i, int j) const {
        if (i < 0 || j < 0) return true;
        if (i >= (int)cells.size()) return true;
        if (j >= (int)cells[i].size()) return true;
        return cells[i][j] != '.';
    }

    bool isSolidAtWorld(float x, float z) const {
        int i = (int)(x / gridSize);
        int j = (int)(z / gridSize);
        return isSolid(i, j);
    }

    bool collidesWithRadius(float x, float z, float radius) const {
        return isSolidAtWorld(x - radius, z - radius)
            || isSolidAtWorld(x + radius, z - radius)
            || isSolidAtWorld(x - radius, z + radius)
            || isSolidAtWorld(x + radius, z + radius);
    }
};

bool loadMap(const char* path, Level& outLevel);