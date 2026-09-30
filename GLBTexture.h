#pragma once
// GLBTexture.h
class GLBTexture {
public:
    GLBTexture();
    ~GLBTexture();

    // Уже декодированные пиксели от TinyGLTF
    bool loadFromPixels(const unsigned char* pixels,
        int width, int height, int channels);

    void bind() const;
    void unbind() const;
    GLuint getId() const { return id; }
    bool isLoaded() const { return loaded; }

private:
    GLuint id = 0;
    bool loaded = false;
};