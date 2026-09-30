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
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть текстуру: " << path << std::endl;
        return false;
    }

    TGAHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(TGAHeader));

    // Поддерживаем только несжатые TrueColor (24/32 бита)
    if (header.imageType != 2) {
        std::cerr << "Поддерживается только несжатый TGA (imageType=2): " << path << std::endl;
        return false;
    }

    width = header.width;
    height = header.height;
    int bpp = header.pixelDepth / 8; // 3 или 4 байта на пиксель

    // Пропускаем ID-секцию, если есть
    if (header.idLength > 0) {
        file.seekg(header.idLength, std::ios::cur);
    }

    std::vector<unsigned char> data(width * height * bpp);
    file.read(reinterpret_cast<char*>(data.data()), data.size());
    file.close();

    // TGA хранит пиксели в BGR(A), OpenGL ждёт RGB(A) — конвертируем
    for (int i = 0; i < width * height; i++) {
        unsigned char b = data[i * bpp + 0];
        unsigned char g = data[i * bpp + 1];
        unsigned char r = data[i * bpp + 2];
        data[i * bpp + 0] = r;
        data[i * bpp + 1] = g;
        data[i * bpp + 2] = b;
    }

    GLenum format = (bpp == 4) ? GL_RGBA : GL_RGB;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
        format, GL_UNSIGNED_BYTE, data.data());

    // Мипмапы — обязательно, иначе дальние стены будут "шумными"
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
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