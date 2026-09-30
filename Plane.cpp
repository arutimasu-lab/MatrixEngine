// Plane.cpp
#include "Plane.h"
#include "TextureManager.h"

Plane::Plane(bool facingUp, float x, float y, float z, float size,
    const std::string& texturePath)
    : facingUp(facingUp), x(x), y(y), z(z), size(size), texturePath(texturePath)
{
    texture = TextureManager::instance().get(texturePath);
}

void Plane::render() {
    if (texture) {
        glEnable(GL_TEXTURE_2D);
        texture->bind();
    }

    glColor3f(1.0f, 1.0f, 1.0f);

    float half = size * 0.5f;

    glBegin(GL_QUADS);
    if (facingUp) {
        // Пол: нормаль вверх, вершины против часовой стрелки если смотреть сверху
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0, 0); glVertex3f(x - half, y, z - half);
        glTexCoord2f(1, 0); glVertex3f(x + half, y, z - half);
        glTexCoord2f(1, 1); glVertex3f(x + half, y, z + half);
        glTexCoord2f(0, 1); glVertex3f(x - half, y, z + half);
    }
    else {
        // Потолок: нормаль вниз, вершины в обратном порядке
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(0, 0); glVertex3f(x - half, y, z + half);
        glTexCoord2f(1, 0); glVertex3f(x + half, y, z + half);
        glTexCoord2f(1, 1); glVertex3f(x + half, y, z - half);
        glTexCoord2f(0, 1); glVertex3f(x - half, y, z - half);
    }
    glEnd();

    if (texture) {
        texture->unbind();
        glDisable(GL_TEXTURE_2D);
    }
}