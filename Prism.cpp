// Prism.cpp
#include "Prism.h"
#include "TextureManager.h"
#include <cmath>

Prism::Prism(float x, float y, float z,
    float size, float depth,
    const std::string& texturePath)
    : x(x), y(y), z(z), size(size), depth(depth),
    texturePath(texturePath)
{
    //texture = TextureManager::instance().get(texturePath);
}

Prism::~Prism() {}

void Prism::setTexture(const std::string& texturePath) {
    this->texturePath = texturePath;
    texture = TextureManager::instance().get(texturePath);
}

void Prism::render() {
    if (texture) {
        glEnable(GL_TEXTURE_2D);
        texture->bind();
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }
    glColor3f(0.1f, 0.5f, 0.9f);  // синий

    float hs = size * 0.5f;    // половина размера (ширина основания)
    float hd = depth * 0.5f;   // половина глубины (высота призмы)

    // Треугольник в плоскости XZ, вершина — впереди (+Z)
    // Передняя вершина (остриё)
    float tipX = x;
    float tipZ = z + hd;

    // Задние углы основания
    float leftX = x - hs;
    float leftZ = z - hd;
    float rightX = x + hs;
    float rightZ = z - hd;

    // Нижняя и верхняя плоскости по Y
    float bottomY = y - hd;
    float topY = y + hd;

    // --- Передняя грань (треугольник, смотрит вперёд) ---
    glBegin(GL_TRIANGLES);

    // Передняя грань (+Z, но нормаль — по направлению к камере)
    glNormal3f(0, 0, 1);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(tipX, topY, tipZ);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(leftX, bottomY, leftZ);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(rightX, bottomY, rightZ);

    // Задняя грань (-Z)
    glNormal3f(0, 0, -1);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(tipX, topY, tipZ);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(rightX, bottomY, rightZ);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(leftX, bottomY, leftZ);

    glEnd();

    // --- Боковые грани (3 прямоугольника) ---
    glBegin(GL_QUADS);

    // Левая грань (между вершиной и левым углом)
    glNormal3f(-0.707f, 0.0f, 0.707f);
    glTexCoord2f(0, 0); glVertex3f(tipX, bottomY, tipZ);
    glTexCoord2f(1, 0); glVertex3f(leftX, bottomY, leftZ);
    glTexCoord2f(1, 1); glVertex3f(leftX, topY, leftZ);
    glTexCoord2f(0, 1); glVertex3f(tipX, topY, tipZ);

    // Правая грань (между вершиной и правым углом)
    glNormal3f(0.707f, 0.0f, 0.707f);
    glTexCoord2f(0, 0); glVertex3f(rightX, bottomY, rightZ);
    glTexCoord2f(1, 0); glVertex3f(tipX, bottomY, tipZ);
    glTexCoord2f(1, 1); glVertex3f(tipX, topY, tipZ);
    glTexCoord2f(0, 1); glVertex3f(rightX, topY, rightZ);

    // Задняя грань (между левым и правым углами)
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(leftX, bottomY, leftZ);
    glTexCoord2f(1, 0); glVertex3f(rightX, bottomY, rightZ);
    glTexCoord2f(1, 1); glVertex3f(rightX, topY, rightZ);
    glTexCoord2f(0, 1); glVertex3f(leftX, topY, leftZ);

    glEnd();

    if (texture) {
        texture->unbind();
        glDisable(GL_TEXTURE_2D);
    }
}