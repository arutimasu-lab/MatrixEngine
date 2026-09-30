#include "ShaderProgram.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

ShaderProgram::ShaderProgram() {}

ShaderProgram::~ShaderProgram() {
    destroy();
}

std::string ShaderProgram::readFile(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Не удалось открыть шейдер: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint ShaderProgram::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Ошибка компиляции шейдера: " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ShaderProgram::loadFromFiles(const char* vertPath, const char* fragPath) {
    std::string vsSrc = readFile(vertPath);
    std::string fsSrc = readFile(fragPath);
    if (vsSrc.empty() || fsSrc.empty()) return false;

    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) return false;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Ошибка линковки шейдера: " << log << std::endl;
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
}

void ShaderProgram::use() const {
    glUseProgram(program);
}

void ShaderProgram::destroy() {
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
}

void ShaderProgram::setMat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE,
        glm::value_ptr(m));
}

void ShaderProgram::setMat4Array(const char* name, const glm::mat4* arr, int count) const {
    glUniformMatrix4fv(glGetUniformLocation(program, name), count, GL_FALSE,
        glm::value_ptr(arr[0]));
}

void ShaderProgram::setVec4(const char* name, const glm::vec4& v) const {
    glUniform4fv(glGetUniformLocation(program, name), 1, glm::value_ptr(v));
}

void ShaderProgram::setInt(const char* name, int v) const {
    glUniform1i(glGetUniformLocation(program, name), v);
}

void ShaderProgram::setBool(const char* name, bool v) const {
    glUniform1i(glGetUniformLocation(program, name), v ? 1 : 0);
}