#include "Cube.h"
#include "TextureManager.h"

Cube::Cube(float x, float y, float z,
    float sx, float sy, float sz,
    const std::string& texturePath)
    : x(x), y(y), z(z),
    sx(sx), sy(sy), sz(sz),
    texturePath(texturePath)
{
    if (!texturePath.empty()) {
        texture = TextureManager::instance().get(texturePath);
    }
}

Cube::~Cube() {}

void Cube::setTexture(const std::string& path) {
    texturePath = path;
    texture = TextureManager::instance().get(path);
}

void Cube::render() {
    if (texture) {
        glEnable(GL_TEXTURE_2D);
        texture->bind();
    }
    glColor3f(1, 1, 1);

    // ѕоловины размеров
    float hx = sx * 0.5f;
    float hy = sy * 0.5f;
    float hz = sz * 0.5f;

    glBegin(GL_QUADS);

    // ¬ерхн€€ грань (+Y) Ч обложка книги
    if (texture) {
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 1); glVertex3f(x - hx, y + hy, z - hz);
        glTexCoord2f(1, 1); glVertex3f(x + hx, y + hy, z - hz);
        glTexCoord2f(1, 0); glVertex3f(x + hx, y + hy, z + hz);
        glTexCoord2f(0, 0); glVertex3f(x - hx, y + hy, z + hz);
    }

    // ќстальные грани Ч без текстуры, коричневые
    glEnd();

    if (texture) {
        texture->unbind();
        glDisable(GL_TEXTURE_2D);
    }

    glColor3f(0.3f, 0.15f, 0.05f);  // тЄмно-коричневый

    glBegin(GL_QUADS);

    // Ќижн€€ грань (-Y)
    glNormal3f(0, -1, 0);
    glVertex3f(x - hx, y - hy, z + hz);
    glVertex3f(x + hx, y - hy, z + hz);
    glVertex3f(x + hx, y - hy, z - hz);
    glVertex3f(x - hx, y - hy, z - hz);

    // ѕередн€€ грань (+Z)
    glNormal3f(0, 0, 1);
    glVertex3f(x - hx, y - hy, z + hz);
    glVertex3f(x + hx, y - hy, z + hz);
    glVertex3f(x + hx, y + hy, z + hz);
    glVertex3f(x - hx, y + hy, z + hz);

    // «адн€€ грань (-Z)
    glNormal3f(0, 0, -1);
    glVertex3f(x + hx, y - hy, z - hz);
    glVertex3f(x - hx, y - hy, z - hz);
    glVertex3f(x - hx, y + hy, z - hz);
    glVertex3f(x + hx, y + hy, z - hz);

    // Ћева€ грань (-X) Ч корешок
    glNormal3f(-1, 0, 0);
    glVertex3f(x - hx, y - hy, z - hz);
    glVertex3f(x - hx, y - hy, z + hz);
    glVertex3f(x - hx, y + hy, z + hz);
    glVertex3f(x - hx, y + hy, z - hz);

    // ѕрава€ грань (+X) Ч срез страниц
    glColor3f(0.9f, 0.85f, 0.7f);  // бежевый Ч страницы
    glNormal3f(1, 0, 0);
    glVertex3f(x + hx, y - hy, z + hz);
    glVertex3f(x + hx, y - hy, z - hz);
    glVertex3f(x + hx, y + hy, z - hz);
    glVertex3f(x + hx, y + hy, z + hz);

    glEnd();
}