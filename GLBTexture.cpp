// GLBTexture.cpp
#include <GL/glew.h>
#include <gl/GL.h>
#include "GLBTexture.h"
#include <iostream>


GLBTexture::GLBTexture() {}

GLBTexture::~GLBTexture() {
    if (id != 0) glDeleteTextures(1, &id);
}

bool GLBTexture::loadFromPixels(const unsigned char* pixels,
    int width, int height, int channels) {
    if (!pixels || width <= 0 || height <= 0) {
        std::cerr << "loadFromPixels: неверные параметры" << std::endl;
        return false;
    }

    GLenum format = GL_RGBA;
    if (channels == 1)      format = GL_RED;
    else if (channels == 2) format = GL_RG;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Выравнивание — важно для RGBA/RGB с шириной, не кратной 4
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
        format, GL_UNSIGNED_BYTE, pixels);

    glGenerateMipmap(GL_TEXTURE_2D); // теперь доступно через GLEW

    glBindTexture(GL_TEXTURE_2D, 0);
    loaded = true;
    return true;
}

void GLBTexture::bind() const {
    if (loaded) glBindTexture(GL_TEXTURE_2D, id);
}

void GLBTexture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}