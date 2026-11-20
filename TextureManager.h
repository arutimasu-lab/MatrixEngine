#pragma once
// TextureManager.h
#include "Texture.h"
#include <map>
#include <string>

class TextureManager {
public:
    static TextureManager& instance();

    Texture* get(const std::string& path);

private:
    TextureManager() = default;//{}
    std::map<std::string, Texture*> cache;
};