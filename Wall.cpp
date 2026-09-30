// Wall.cpp
#include "Wall.h"
#include "TextureManager.h"

Wall::Wall(WallType type, float x, float y, float z, float size,
    const std::string& texturePath)
    : type(type), x(x), y(y), z(z), size(size), texturePath(texturePath)
{
    texture = TextureManager::instance().get(texturePath);
}

void Wall::render() {
    if (texture) {
        glEnable(GL_TEXTURE_2D);
        texture->bind();
    }
    glColor3f(1.0f, 1.0f, 1.0f);

    switch (type) {
    case WallType::Full:       renderFull();       break;
    case WallType::Horizontal: renderHorizontal(); break;
    case WallType::Vertical:   renderVertical();   break;
    }

    if (texture) {
        texture->unbind();
        glDisable(GL_TEXTURE_2D);
    }
}

void Wall::renderHorizontal() {
    // Грань вдоль Z: стена стоит поперёк X, тянется вдоль Z
    float h = size * 0.5f;
    float y0 = y - h;
    float y1 = y + h;

    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(x - h, y0, z + h);
    glTexCoord2f(1, 0); glVertex3f(x + h, y0, z + h);
    glTexCoord2f(1, 1); glVertex3f(x + h, y1, z + h);
    glTexCoord2f(0, 1); glVertex3f(x - h, y1, z + h);
    glEnd();
}

void Wall::renderVertical() {
    // Грань вдоль X: стена стоит поперёк Z, тянется вдоль X
    float h = size * 0.5f;
    float y0 = y - h;
    float y1 = y + h;

    glBegin(GL_QUADS);
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(x + h, y0, z - h);
    glTexCoord2f(1, 0); glVertex3f(x + h, y0, z + h);
    glTexCoord2f(1, 1); glVertex3f(x + h, y1, z + h);
    glTexCoord2f(0, 1); glVertex3f(x + h, y1, z - h);
    glEnd();
}

void Wall::renderFull() {
    float h = size * 0.5f;
    float y0 = y - h;
    float y1 = y + h;

    glBegin(GL_QUADS);

    // +Z
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(x - h, y0, z + h);
    glTexCoord2f(1, 0); glVertex3f(x + h, y0, z + h);
    glTexCoord2f(1, 1); glVertex3f(x + h, y1, z + h);
    glTexCoord2f(0, 1); glVertex3f(x - h, y1, z + h);

    // -Z
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(x + h, y0, z - h);
    glTexCoord2f(1, 0); glVertex3f(x - h, y0, z - h);
    glTexCoord2f(1, 1); glVertex3f(x - h, y1, z - h);
    glTexCoord2f(0, 1); glVertex3f(x + h, y1, z - h);

    // +X
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(x + h, y0, z + h);
    glTexCoord2f(1, 0); glVertex3f(x + h, y0, z - h);
    glTexCoord2f(1, 1); glVertex3f(x + h, y1, z - h);
    glTexCoord2f(0, 1); glVertex3f(x + h, y1, z + h);

    // -X
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(x - h, y0, z - h);
    glTexCoord2f(1, 0); glVertex3f(x - h, y0, z + h);
    glTexCoord2f(1, 1); glVertex3f(x - h, y1, z + h);
    glTexCoord2f(0, 1); glVertex3f(x - h, y1, z - h);

    glEnd();
}