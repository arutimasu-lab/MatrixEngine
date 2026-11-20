#include "Map.h"

#include <fstream>
#include <sstream>
#include <iostream>
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
    enum Section { NONE, OBJECTS, MODELS, MAP, ROAD } section = NONE;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line == "[OBJECTS]") { section = OBJECTS; continue; }
        if (line == "[MODELS]") { section = MODELS;  continue; }
        if (line == "[MAP]") { section = MAP;     continue; }
        if (line == "[ROAD]") { section = ROAD;    continue; }

        std::istringstream iss(line);

        if (section == OBJECTS) {
            char code;
            std::string eq, name;
            iss >> code >> eq >> name;
            if (eq == "=") outLevel.objects[code] = name;
        }
        else if (section == MODELS) {
            char code;
            std::string eq, name;
            iss >> code >> eq >> name;
            if (eq == "=") outLevel.models[code] = name;
        }
        else if (section == MAP) {
            std::vector<char> row;
            std::string token;
            while (iss >> token) {
                for (char c : token) row.push_back(c);
            }
            if (!row.empty()) outLevel.cells.push_back(row);
        }
        else if (section == ROAD) {
            std::vector<char> row;
            std::string token;
            while (iss >> token) {
                for (char c : token) row.push_back(c);
            }
            if (!row.empty()) outLevel.roadCells.push_back(row);
        }
    }

    // ========================================================
    // Проверяем, что карта вообще существует
    // ========================================================

    if (outLevel.cells.empty())
    {
        std::cerr << "Карта пуста: "
            << path << std::endl;

        return false;
    }


    // ========================================================
    // Ищем игрока (P) и машину (C)
    // ========================================================

    for (size_t i = 0; i < outLevel.cells.size(); i++)
    {
        for (size_t j = 0; j < outLevel.cells[i].size(); j++)
        {
            if (outLevel.cells[i][j] == 'P')
            {
                outLevel.spawnI = (int)i;
                outLevel.spawnJ = (int)j;

                // Игрок занимает пустую клетку
                outLevel.cells[i][j] = '.';
            }

            if (outLevel.cells[i][j] == 'C')
            {
                outLevel.carSpawnI = (int)i;
                outLevel.carSpawnJ = (int)j;

                // Машина занимает пустую клетку
                outLevel.cells[i][j] = '.';
            }
        }
    }


    // ========================================================
    // Расставляем OBJ-модели
    // ========================================================
    for (size_t i = 0; i < outLevel.cells.size(); ++i) {
        for (size_t j = 0; j < outLevel.cells[i].size(); ++j) {

            char code = outLevel.cells[i][j];

            auto modelIt = outLevel.models.find(code);

            if (modelIt == outLevel.models.end())
                continue;

            LevelObject object;

            object.modelPath = modelIt->second;

            // ВАЖНО:
            // Старая система координат движка:
            //
            // i -> X
            // j -> Z
            //
            object.x = (static_cast<float>(i) + 0.5f)
                * outLevel.gridSize;

            object.y = 0.0f;

            object.z = (static_cast<float>(j) + 0.5f)
                * outLevel.gridSize;

            outLevel.placedObjects.push_back(object);
        }
    }


    // ========================================================
    // Информация для отладки
    // ========================================================

    std::cout << "Карта загружена: "
        << path << std::endl;

    std::cout << "Размер карты: "
        << outLevel.cells.size()
        << " x ";

    size_t maxWidth = 0;

    for (const auto& row : outLevel.cells)
    {
        if (row.size() > maxWidth)
            maxWidth = row.size();
    }

    std::cout << maxWidth << std::endl;


    std::cout << "Текстурных объектов: "
        << outLevel.objects.size()
        << std::endl;


    std::cout << "OBJ моделей: "
        << outLevel.models.size()
        << std::endl;


    std::cout << "Размещённых OBJ: "
        << outLevel.placedObjects.size()
        << std::endl;


    if (outLevel.spawnI >= 0 && outLevel.spawnJ >= 0)
    {
        std::cout << "Spawn player: "
            << outLevel.spawnI
            << ", "
            << outLevel.spawnJ
            << std::endl;
    }

    if (outLevel.carSpawnI >= 0 && outLevel.carSpawnJ >= 0)
    {
        std::cout << "Spawn car: "
            << outLevel.carSpawnI
            << ", "
            << outLevel.carSpawnJ
            << std::endl;
    }


    return true;
}