#pragma once
// TextureManager.h
#pragma once
#include "Texture.h"
#include <map>
#include <string>

class TextureManager {
public:
    static TextureManager& instance();

    Texture* get(const std::string& path);

private:
    TextureManager() {}
    std::map<std::string, Texture*> cache;
};