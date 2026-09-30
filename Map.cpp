// Map.cpp
#include "Map.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool loadMap(const char* path, Level& outLevel) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть карту: " << path << std::endl;
        return false;
    }

    std::string line;
    enum Section { NONE, OBJECTS, MAP } section = NONE;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line == "[OBJECTS]") { section = OBJECTS; continue; }
        if (line == "[MAP]") { section = MAP;     continue; }

        std::istringstream iss(line);

        if (section == OBJECTS) {
            // Формат: H = wall_stone_h
            char code;
            std::string eq, name;
            iss >> code >> eq >> name;
            if (eq == "=") {
                outLevel.objects[code] = name;
            }
        }
        else if (section == MAP) {
            // Формат: S S S S или H.V.H
            std::vector<char> row;
            std::string token;
            while (iss >> token) {
                // Поддерживаем и "H V H", и "HVH"
                for (char c : token) {
                    row.push_back(c);
                }
            }
            if (!row.empty()) {
                outLevel.cells.push_back(row);
            }
        }
    }

    if (outLevel.cells.empty()) {
        std::cerr << "Карта пуста: " << path << std::endl;
        return false;
    }

    // Ищем спавн игрока
    for (size_t i = 0; i < outLevel.cells.size(); i++) {
        for (size_t j = 0; j < outLevel.cells[i].size(); j++) {
            if (outLevel.cells[i][j] == 'P') {
                outLevel.spawnI = (int)i;
                outLevel.spawnJ = (int)j;
                outLevel.cells[i][j] = '.'; // убираем маркер
            }
        }
    }

    return true;
}