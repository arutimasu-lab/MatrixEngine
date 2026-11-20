#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>

//#define STB_IMAGE_IMPLEMENTATION
#include "tinygltf/stb_image.h"
#include "ModelLoader.h"


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>



using namespace std;


// ------------------------------------------------------------
// Вспомогательная функция: получить папку файла
// ------------------------------------------------------------

static string getDirectory(const string& path)
{
    size_t slash = path.find_last_of("/\\");

    if (slash == string::npos)
        return "";

    return path.substr(0, slash + 1);
}


// ------------------------------------------------------------
// Загрузка текстуры
// ------------------------------------------------------------

unsigned int Model::loadTexture(const string& path)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* data = stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        0
    );

    if (!data)
    {
        cerr << "Не удалось загрузить текстуру: "
            << path << endl;

        return 0;
    }

    GLenum format = GL_RGB;

    if (channels == 4)
        format = GL_RGBA;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 1)
        format = GL_LUMINANCE;

    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    cout << "Загружена текстура: "
        << path
        << " ("
        << width
        << "x"
        << height
        << ")"
        << endl;

    return texture;
}


// ------------------------------------------------------------
// Загрузка MTL
// ------------------------------------------------------------

bool Model::loadMTL(const string& path)
{
    ifstream file(path);

    if (!file.is_open())
    {
        cerr << "Не удалось открыть MTL: "
            << path << endl;

        return false;
    }

    string directory = getDirectory(path);

    string line;
    Material* currentMaterial = nullptr;

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        istringstream s(line);

        string prefix;
        s >> prefix;

        // ----------------------------------------------------
        // newmtl
        // ----------------------------------------------------

        if (prefix == "newmtl")
        {
            string name;
            s >> name;

            Material material;
            material.name = name;

            materials.push_back(material);

            int index = static_cast<int>(materials.size()) - 1;

            materialMap[name] = index;

            currentMaterial = &materials[index];
        }

        // ----------------------------------------------------
        // Kd — основной цвет
        // ----------------------------------------------------

        else if (prefix == "Kd" && currentMaterial)
        {
            s >> currentMaterial->r
                >> currentMaterial->g
                >> currentMaterial->b;
        }

        // ----------------------------------------------------
        // map_Kd — diffuse texture
        // ----------------------------------------------------

        else if (prefix == "map_Kd" && currentMaterial)
        {
            string texturePath;
            s >> texturePath;

            string fullPath = directory + texturePath;

            currentMaterial->texture =
                loadTexture(fullPath);

            if (currentMaterial->texture == 0)
            {
                cerr << "Не удалось загрузить map_Kd: "
                    << fullPath << endl;
            }
        }
    }

    file.close();

    cout << "Загружен MTL: "
        << path
        << " | материалов: "
        << materials.size()
        << endl;

    return true;
}


// ------------------------------------------------------------
// Загрузка OBJ
// ------------------------------------------------------------

bool Model::loadOBJ(const char* path)
{
    ifstream file(path);

    if (!file.is_open())
    {
        cerr << "Не удалось открыть файл: "
            << path << endl;

        return false;
    }

    // На случай повторной загрузки
    vertices.clear();
    texCoords.clear();
    faces.clear();
    materials.clear();
    materialMap.clear();

    string objPath = path;
    string directory = getDirectory(objPath);

    int currentMaterial = -1;

    string line;

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        istringstream s(line);

        string prefix;
        s >> prefix;


        // ----------------------------------------------------
        // mtllib
        // ----------------------------------------------------

        if (prefix == "mtllib")
        {
            string mtlFile;

            s >> mtlFile;

            string mtlPath = directory + mtlFile;

            loadMTL(mtlPath);
        }


        // ----------------------------------------------------
        // v
        // ----------------------------------------------------

        else if (prefix == "v")
        {
            Vertex vertex;

            s >> vertex.x
                >> vertex.y
                >> vertex.z;

            vertices.push_back(vertex);
        }


        // ----------------------------------------------------
        // vt
        // ----------------------------------------------------

        else if (prefix == "vt")
        {
            TexCoord texCoord;

            s >> texCoord.u
                >> texCoord.v;

            // OBJ и OpenGL имеют разные направления Y
            texCoord.v = 1.0f - texCoord.v;

            texCoords.push_back(texCoord);
        }


        // ----------------------------------------------------
        // usemtl
        // ----------------------------------------------------

        else if (prefix == "usemtl")
        {
            string materialName;

            s >> materialName;

            auto it = materialMap.find(materialName);

            if (it != materialMap.end())
            {
                currentMaterial = it->second;
            }
            else
            {
                cerr << "Материал не найден: "
                    << materialName << endl;

                currentMaterial = -1;
            }
        }


        // ----------------------------------------------------
        // f
        // ----------------------------------------------------

        else if (prefix == "f")
        {
            vector<int> vertexIndices;
            vector<int> texCoordIndices;

            string vertexIndexStr;

            while (s >> vertexIndexStr)
            {
                int vertexIndex = -1;
                int texCoordIndex = -1;

                size_t slash1 =
                    vertexIndexStr.find('/');

                if (slash1 == string::npos)
                {
                    // f 1 2 3

                    vertexIndex =
                        stoi(vertexIndexStr) - 1;
                }
                else
                {
                    // Получаем индекс вершины

                    string vertexPart =
                        vertexIndexStr.substr(
                            0,
                            slash1
                        );

                    vertexIndex =
                        stoi(vertexPart) - 1;

                    // Ищем второй /

                    size_t slash2 =
                        vertexIndexStr.find(
                            '/',
                            slash1 + 1
                        );

                    if (slash2 == string::npos)
                    {
                        // v/vt

                        string texPart =
                            vertexIndexStr.substr(
                                slash1 + 1
                            );

                        if (!texPart.empty())
                        {
                            texCoordIndex =
                                stoi(texPart) - 1;
                        }
                    }
                    else
                    {
                        // v/vt/vn

                        string texPart =
                            vertexIndexStr.substr(
                                slash1 + 1,
                                slash2 - slash1 - 1
                            );

                        if (!texPart.empty())
                        {
                            texCoordIndex =
                                stoi(texPart) - 1;
                        }
                    }
                }

                vertexIndices.push_back(vertexIndex);
                texCoordIndices.push_back(texCoordIndex);
            }


            // ------------------------------------------------
            // Триангуляция polygon -> triangles
            // ------------------------------------------------

            for (size_t i = 1;
                i + 1 < vertexIndices.size();
                ++i)
            {
                Face triangle;

                triangle.vertexIndices.push_back(
                    vertexIndices[0]
                );

                triangle.vertexIndices.push_back(
                    vertexIndices[i]
                );

                triangle.vertexIndices.push_back(
                    vertexIndices[i + 1]
                );


                if (!texCoordIndices.empty())
                {
                    triangle.texCoordIndices.push_back(
                        texCoordIndices[0]
                    );

                    triangle.texCoordIndices.push_back(
                        texCoordIndices[i]
                    );

                    triangle.texCoordIndices.push_back(
                        texCoordIndices[i + 1]
                    );
                }

                triangle.materialIndex =
                    currentMaterial;

                faces.push_back(triangle);
            }
        }
    }

    file.close();

    cout << "OBJ загружен: "
        << path
        << endl;

    cout << "  vertices: "
        << vertices.size()
        << endl;

    cout << "  texcoords: "
        << texCoords.size()
        << endl;

    cout << "  triangles: "
        << faces.size()
        << endl;

    cout << "  materials: "
        << materials.size()
        << endl;

    return true;
}


// ------------------------------------------------------------
// Рендер
// ------------------------------------------------------------

void Model::render()
{
    glEnable(GL_TEXTURE_2D);

    for (const Face& face : faces)
    {
        Material* material = nullptr;

        if (face.materialIndex >= 0 &&
            face.materialIndex <
            static_cast<int>(materials.size()))
        {
            material =
                &materials[face.materialIndex];
        }


        // ----------------------------------------------------
        // Материал
        // ----------------------------------------------------

        if (material)
        {
            glColor3f(
                material->r,
                material->g,
                material->b
            );

            if (material->texture != 0)
            {
                glBindTexture(
                    GL_TEXTURE_2D,
                    material->texture
                );
            }
            else
            {
                glBindTexture(
                    GL_TEXTURE_2D,
                    0
                );
            }
        }
        else
        {
            glColor3f(
                1.0f,
                1.0f,
                1.0f
            );

            glBindTexture(
                GL_TEXTURE_2D,
                0
            );
        }


        // ----------------------------------------------------
        // Треугольник
        // ----------------------------------------------------

        glBegin(GL_TRIANGLES);

        for (size_t i = 0;
            i < face.vertexIndices.size();
            ++i)
        {
            int vertexIndex =
                face.vertexIndices[i];

            if (vertexIndex < 0 ||
                vertexIndex >=
                static_cast<int>(vertices.size()))
            {
                continue;
            }

            // UV
            if (i < face.texCoordIndices.size())
            {
                int texIndex =
                    face.texCoordIndices[i];

                if (texIndex >= 0 &&
                    texIndex <
                    static_cast<int>(texCoords.size()))
                {
                    glTexCoord2f(
                        texCoords[texIndex].u,
                        texCoords[texIndex].v
                    );
                }
            }

            const Vertex& vertex =
                vertices[vertexIndex];

            glVertex3f(
                vertex.x + x,
                vertex.y + y,
                vertex.z + z
            );
        }

        glEnd();
    }

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    glDisable(GL_TEXTURE_2D);

    glColor3f(
        1.0f,
        1.0f,
        1.0f
    );
}