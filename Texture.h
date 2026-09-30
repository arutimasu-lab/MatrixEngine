#pragma once
// Texture.h
#pragma once
#include <GL/glut.h>
#include <string>

class Texture {
public:
    Texture();
    ~Texture();

    bool load(const char* path);
    void bind() const;
    void unbind() const;

    GLuint getId() const { return id; }
    bool isLoaded() const { return loaded; }

private:
    GLuint id = 0;
    bool loaded = false;
    int width = 0;
    int height = 0;
};