#pragma once
// GLBLoader.h
#include <string>

// Только объявление — без IMPLEMENTATION

class GLBLoader {
public:
    static bool load(const std::string& path, tinygltf::Model& outModel);
};