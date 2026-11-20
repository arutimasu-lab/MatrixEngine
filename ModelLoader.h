#pragma once

#include <string>
#include <vector>
#include <map>

struct Vertex
{
    float x, y, z;
};

struct TexCoord
{
    float u, v;
};

struct Material
{
    std::string name;

    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;

    unsigned int texture = 0;
};

struct Face
{
    std::vector<int> vertexIndices;
    std::vector<int> texCoordIndices;

    int materialIndex = -1;
};

class Model
{
public:
    float x;
    float y;
    float z;

    Model(float px, float py, float pz)
        : x(px), y(py), z(pz)
    {
    }

    bool loadOBJ(const char* path);
    void render();

private:
    bool loadMTL(const std::string& path);
    unsigned int loadTexture(const std::string& path);

    std::vector<Vertex> vertices;
    std::vector<TexCoord> texCoords;
    std::vector<Face> faces;

    std::vector<Material> materials;
    std::map<std::string, int> materialMap;
};;