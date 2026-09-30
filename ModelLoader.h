#pragma once
#include <vector>
using namespace std;

struct Vertex {
    float x, y, z;
};

struct Face {
    vector<int> vertexIndices;
};

class Model {
public:
    Model(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    bool loadOBJ(const char* path);
    void render();
private:
    int x, y, z;
    vector<Vertex> vertices;
    vector<Face> faces;
};


//void renderCube(int x, int y, int z);
//void renderModel();
//void renderLandscape();
//bool loadOBJ(const char* path);