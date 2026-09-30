#pragma once
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    bool loadFromFiles(const char* vertPath, const char* fragPath);
    void use() const;
    void destroy();

    GLuint getId() const { return program; }

    // Установка uniform'ов
    void setMat4(const char* name, const glm::mat4& m) const;
    void setMat4Array(const char* name, const glm::mat4* arr, int count) const;
    void setVec4(const char* name, const glm::vec4& v) const;
    void setInt(const char* name, int v) const;
    void setBool(const char* name, bool v) const;

private:
    GLuint program = 0;

    static GLuint compileShader(GLenum type, const std::string& source);
    static std::string readFile(const char* path);
};