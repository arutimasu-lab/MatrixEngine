#define _CRT_SECURE_NO_WARNINGS

#include "ModelLoader.h"
#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;




bool Model::loadOBJ(const char* path) {
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл: " << path << endl;
        return false;
    }

    string line;
    while (getline(file, line)) {
        istringstream s(line);
        string prefix;
        s >> prefix;

        if (prefix == "v") {
            Vertex vertex;
            s >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        }
        else if (prefix == "f") {
            Face face;
            string vertexIndexStr;
            while (s >> vertexIndexStr) {
                size_t pos = vertexIndexStr.find('/');
                int vertexIndex = stoi(vertexIndexStr.substr(0, pos)) - 1; // Индексы начинаются с 1
                face.vertexIndices.push_back(vertexIndex);
            }
            // Разбиение полигона на треугольники с помощью первой вершины
            for (size_t i = 1; i < face.vertexIndices.size() - 1; ++i) {
                Face triangle;
                triangle.vertexIndices.push_back(face.vertexIndices[0]);
                triangle.vertexIndices.push_back(face.vertexIndices[i]);
                triangle.vertexIndices.push_back(face.vertexIndices[i + 1]);
                faces.push_back(triangle);
            }
        }
    }
    file.close();
    return true;
}

void Model::render() {
    glBegin(GL_TRIANGLES);
    for (const Face& face : faces) {
        for (int index : face.vertexIndices) {
            glVertex3f(vertices[index].x + x, vertices[index].y + y, vertices[index].z + z);
        }
    }
    glEnd();
}