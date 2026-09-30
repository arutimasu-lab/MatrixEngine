// GLBLoader.cpp — реализация tinygltf ТОЛЬКО здесь
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"
#include "GLBLoader.h"
#include <iostream>

bool GLBLoader::load(const std::string& path, tinygltf::Model& outModel) {
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    loader.SetStoreOriginalJSONForExtrasAndExtensions(true);
    // GLB — бинарный формат. Для .gltf используйте LoadASCIIFromFile
    bool ok = loader.LoadBinaryFromFile(&outModel, &err, &warn, path);

    if (!warn.empty()) {
        std::cerr << "tinygltf warn: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "tinygltf err: " << err << std::endl;
    }
    if (!ok) {
        std::cerr << "Не удалось загрузить GLB: " << path << std::endl;
    }
    return ok;
}