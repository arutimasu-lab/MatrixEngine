// TextureManager.cpp
#include "TextureManager.h"
#include <iostream>

TextureManager& TextureManager::instance() {
    static TextureManager mgr;
    return mgr;
}

Texture* TextureManager::get(const std::string& path) {
    auto it = cache.find(path);
    if (it != cache.end()) {
        return it->second;
    }

    Texture* tex = new Texture();
    if (!tex->load(path.c_str())) {
        delete tex;
        return nullptr;
    }
    cache[path] = tex;
    return tex;
}