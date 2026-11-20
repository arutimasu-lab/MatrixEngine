// Texture.cpp
#include <GL/glew.h>
#include "Texture.h"
#include <fstream>
#include <iostream>
#include <vector>


#pragma pack(push, 1)
struct TGAHeader {
    unsigned char  idLength;
    unsigned char  colorMapType;
    unsigned char  imageType;
    unsigned short colorMapStart;
    unsigned short colorMapLength;
    unsigned char  colorMapDepth;
    unsigned short xOrigin;
    unsigned short yOrigin;
    unsigned short width;
    unsigned short height;
    unsigned char  pixelDepth;
    unsigned char  imageDescriptor;
};
#pragma pack(pop)

Texture::Texture() {}

Texture::~Texture() {
    if (id != 0) {
        glDeleteTextures(1, &id);
    }
}

bool Texture::load(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    TGAHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(TGAHeader));

    if (header.imageType != 2) {
        std::cerr << "Только uncompressed TGA (imageType=2)" << std::endl;
        return false;
    }

    width = header.width;
    height = header.height;
    int bpp = header.pixelDepth / 8;

    if (header.idLength > 0) file.seekg(header.idLength, std::ios::cur);

    std::vector<unsigned char> data(width * height * bpp);
    file.read(reinterpret_cast<char*>(data.data()), data.size());
    file.close();

    // BGR(A) -> RGB(A)
    for (int i = 0; i < width * height; i++) {
        std::swap(data[i * bpp + 0], data[i * bpp + 2]);
    }

    GLenum format = (bpp == 4) ? GL_RGBA : GL_RGB;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // --- ФИЛЬТРАЦИЯ ---
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // --- АНИЗОТРОПИЯ ---
    if (GLEW_EXT_texture_filter_anisotropic) {
        GLfloat maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
        fprintf(stderr, "Aniso: %.1fx\n", maxAniso);
    }

    // --- ЗАГРУЗКА ---
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
        format, GL_UNSIGNED_BYTE, data.data());

    // --- МИПМАПЫ ---
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    fprintf(stderr, "Texture loaded: %dx%d\n", width, height);
    loaded = true;
    return true;
}

void Texture::bind() const {
    if (loaded) {
        glBindTexture(GL_TEXTURE_2D, id);
    }
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}