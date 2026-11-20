#define _CRT_SECURE_NO_WARNINGS

#include <GL/glew.h>
#include <GL/glut.h>

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <clocale>
#include <memory>
#include <windows.h>

#include "ModelLoader.h"
#include "Cube.h"
#include "Wall.h"
#include "Terrain.h"
#include "Camera.h"
#include "Map.h"
#include "Plane.h"
#include "GLBModel.h"
#include "GLBLoader.h"
#include "Prism.h"
#include "TextureManager.h"

using namespace std;

// ============================================================
// 1. СОСТОЯНИЯ ИГРЫ
// ============================================================

enum GamePhase {
    PHASE_PLAYING,
    PHASE_DIALOGUE
};

GamePhase gamePhase = PHASE_PLAYING;

// ============================================================
// 1.1. УРОВНИ
// ============================================================

enum LevelId {
    LEVEL_LIBRARY = 1,
    LEVEL_SCHOOL = 2,
    LEVEL_EMPTY = 3,
    LEVEL_CROSSROAD = 4
};

LevelId currentLevel = LEVEL_CROSSROAD;

// ============================================================
// 1.2. КОНЦОВКИ
// ============================================================

enum EndingType {
    ENDING_NONE = 0,
    ENDING_BAD,
    ENDING_GOOD
};

EndingType pendingEnding = ENDING_NONE;
EndingType currentEnding = ENDING_NONE;
bool gameEnded = false;

// ============================================================
// 2. ИНДЕКСЫ АНИМАЦИЙ
// ============================================================

const int NPC_CAUGHT_ANIMATION = 0;
const int NPC_WALK_ANIMATION = 1;

// ============================================================
// 3. ДИАЛОГ
// ============================================================

struct Choice {
    std::string text;
    int result;
};

struct DialogueLine {
    std::string speaker;
    std::string text;
    bool isChoice = false;
    std::vector<Choice> choices;
};

struct DialogueSequence {
    std::vector<DialogueLine> lines;
    bool loadSchoolAfter = false;
    bool active = false;
    bool waitingChoice = false;
    int currentLine = 0;
};

DialogueSequence currentDialogue;

// ============================================================
// 4. WAYPOINT
// ============================================================

struct Waypoint {
    float x;
    float z;
};

// ============================================================
// 5. ГЛОБАЛЬНЫЕ ОБЪЕКТЫ
// ============================================================

GLBModel npcModel;
GLBModel carModel;

Cube* book = nullptr;
Camera camera;

bool mouseCaptured = false;

Level lvl;

std::vector<std::unique_ptr<Model>> levelModels;

// ============================================================
// 5.1. КНИГА
// ============================================================

float bookX = 7.0f;
float bookY = 0.7f;
float bookZ = 12.9f;

bool bookPicked = false;
bool hasBookInfo = false;

// ============================================================
// 5.2. СОСТОЯНИЕ МАИ
// ============================================================

bool npcWaiting = false;

// ============================================================
// 5.3. МАШИНА
// ============================================================

float carX = -50.0f;
float carY = 0.5f;
float carZ = 0.0f;
float carAngle = 0.0f;
float carSpeed = 3.0f;
bool carActive = false;
bool carTriggered = false;
bool carModelLoaded = false;
bool carDebug = true;
float carScale = 1.0f;
float carDebugLogTimer = 0.0f;

// ============================================================
// 6. ПУТИ ПАТРУЛИРОВАНИЯ
// ============================================================

std::vector<std::vector<Waypoint>> libraryPaths = {
    { { 3.0f,  7.0f },{ 3.0f, 13.0f } },
    { { 7.0f,  7.0f },{ 7.0f, 13.0f } },
    { { 11.0f, 7.0f },{ 11.0f, 13.0f } }
};

std::vector<std::vector<Waypoint>> schoolPaths = {
    { { 3.0f,  3.0f },{ 3.0f, 5.0f } },
    { { 4.0f,  9.0f },{ 4.0f, 11.0f } },
    { { 5.0f, 15.0f },{ 5.0f, 17.0f } }
};

std::vector<std::vector<Waypoint>> npcPaths = libraryPaths;

int currentPath = 0;
int currentWaypoint = 1;

bool npcForward = true;

float npcSpeed = 2.0f;

float npcX = 0.0f;
float npcY = 0.0f;
float npcZ = 0.0f;

float npcAngle = 0.0f;

bool npcCaught = false;
bool npcWasCaught = false;

float npcCatchDistance = 1.5f;

// ============================================================
// 7. КЛАВИАТУРА И ТАЙМИНГ
// ============================================================

bool keyW = false;
bool keyA = false;
bool keyS = false;
bool keyD = false;

float lastTime = 0.0f;

int frameCount = 0;
float fpsTimer = 0.0f;

// ============================================================
// 8. ЗАГРУЗКА OBJ-МОДЕЛЕЙ
// ============================================================

void loadLevelModels()
{
    levelModels.clear();

    for (const LevelObject& object : lvl.placedObjects) {
        std::string path = object.modelPath;

        if (path.rfind("resources/", 0) != 0) {
            path = "resources/" + path;
        }

        std::cout << "Загрузка OBJ: " << path
            << " at (" << object.x << ", "
            << object.y << ", " << object.z << ")" << std::endl;

        std::unique_ptr<Model> model(
            new Model(object.x, object.y, object.z)
        );

        model->loadOBJ(path.c_str());
        levelModels.push_back(std::move(model));
    }

    std::cout << "OBJ моделей загружено: "
        << levelModels.size() << std::endl;
}

// ============================================================
// 9. ФУНКЦИИ NPC
// ============================================================

void teleportNPCToPath(int pathIndex, bool fromStart)
{
    if (pathIndex < 0 ||
        pathIndex >= static_cast<int>(npcPaths.size())) {
        return;
    }

    currentPath = pathIndex;

    const auto& path = npcPaths[currentPath];

    if (path.size() < 2)
        return;

    if (fromStart) {
        currentWaypoint = 1;
        npcForward = true;
        npcX = path[0].x;
        npcZ = path[0].z;
    }
    else {
        currentWaypoint = 0;
        npcForward = false;
        npcX = path[1].x;
        npcZ = path[1].z;
    }
}

void chooseNextPatrolPath()
{
    if (npcPaths.empty())
        return;

    int newPath = currentPath;

    if (npcPaths.size() > 1) {
        do {
            newPath = rand() % static_cast<int>(npcPaths.size());
        } while (newPath == currentPath);
    }

    currentPath = newPath;

    const auto& path = npcPaths[currentPath];

    if (path.size() < 2)
        return;

    if (rand() % 2 == 0) {
        npcX = path[0].x;
        npcZ = path[0].z;
        currentWaypoint = 1;
        npcForward = true;
    }
    else {
        npcX = path[1].x;
        npcZ = path[1].z;
        currentWaypoint = 0;
        npcForward = false;
    }
}

// ============================================================
// 10. ЗАГРУЗКА УРОВНЯ
// ============================================================

void loadLevel(LevelId levelId)
{
    currentLevel = levelId;

    if (book) {
        delete book;
        book = nullptr;
    }

    // === ПУСТОЙ УРОВЕНЬ ===
    if (levelId == LEVEL_EMPTY) {
        lvl.cells.clear();
        lvl.objects.clear();
        lvl.models.clear();
        lvl.placedObjects.clear();
        lvl.roadCells.clear();
        lvl.spawnI = -1;
        lvl.spawnJ = -1;
        levelModels.clear();

        camera.setPosition(0.0f, 1.4f, 0.0f);
        camera.setRotation(0.0f, 0.0f);

        npcX = 1000.0f;
        npcZ = 1000.0f;
        npcWaiting = false;
        npcCaught = true;
        npcWasCaught = true;

        carActive = false;
        carTriggered = false;

        std::printf("Уровень 'Пустота' загружен\n");
        return;
    }

    // === ВЫБОР КАРТЫ ===
    const char* mapPath = nullptr;

    switch (levelId) {
    case LEVEL_LIBRARY:
        mapPath = "resources/maze01.map";
        break;
    case LEVEL_SCHOOL:
        mapPath = "resources/school.map";
        break;
    case LEVEL_CROSSROAD:
        mapPath = "resources/crossroad.map";
        break;
    default:
        std::cerr << "Неизвестный уровень: " << (int)levelId << std::endl;
        return;
    }

    if (!loadMap(mapPath, lvl)) {
        std::cerr << "Ошибка загрузки карты: " << mapPath << std::endl;
        return;
    }

    std::printf("roadCells: %zu строк\n", lvl.roadCells.size());

    loadLevelModels();

    if (lvl.spawnI >= 0 && lvl.spawnJ >= 0) {
        float gs = lvl.gridSize;
        camera.setPosition((lvl.spawnI + 0.5f) * gs, 1.4f,
            (lvl.spawnJ + 0.5f) * gs);
        camera.setRotation(0.0f, 0.0f);
    }

    npcCaught = false;
    npcWasCaught = false;

    if (levelId == LEVEL_LIBRARY) {
        npcPaths = libraryPaths;
        npcWaiting = false;
        npcModel.setAnimation(NPC_WALK_ANIMATION);
        teleportNPCToPath(0, true);
        std::printf("Уровень 'Библиотека' загружен\n");
    }
    else if (levelId == LEVEL_SCHOOL) {
        npcPaths = schoolPaths;
        npcWaiting = true;
        npcX = 5.0f;
        npcZ = 15.0f;
        npcAngle = 0.0f;
        npcModel.setAnimation(NPC_CAUGHT_ANIMATION);

        book = new Cube(bookX, bookY, bookZ,
            0.7f, 0.08f, 1.0f,
            "resources/book_cover.tga");
        bookPicked = false;
        hasBookInfo = false;

        std::printf("Уровень 'Школа' загружен\n");
    }
    else if (levelId == LEVEL_CROSSROAD) {
        npcPaths.clear();
        npcWaiting = false;
        npcCaught = true;
        npcWasCaught = true;
        npcX = 1000.0f;
        npcZ = 1000.0f;

        // Сброс машины. На перекрёстке машина теперь появляется сразу,
        // а не ждёт, пока игрок наступит на клетку дороги.
        carTriggered = false;
        carActive = false;

        // Спавн машины из карты. Координаты карты переводятся в мировые.
        if (lvl.carSpawnI >= 0 && lvl.carSpawnJ >= 0) {
            const float gs = lvl.gridSize;
            carX = (lvl.carSpawnI + 0.5f) * gs;
            carZ = (lvl.carSpawnJ + 0.5f) * gs;
            carY = 0.08f;
            carAngle = 0.0f;
            std::printf("[CAR] spawn из карты: cell=(%d,%d), grid=%.3f, world=(%.2f, %.2f, %.2f)\n",
                lvl.carSpawnI, lvl.carSpawnJ, gs, carX, carY, carZ);
        }
        else {
            // Надёжный fallback: ставим машину перед игроком, немного правее.
            carX = camera.getX() + 4.0f;
            carZ = camera.getZ() + 4.0f;
            carY = 0.08f;
            carAngle = 0.0f;
            std::printf("[CAR][WARN] carSpawn не задан в crossroad.map; fallback world=(%.2f, %.2f, %.2f)\n",
                carX, carY, carZ);
        }

        carActive = carModelLoaded;
        carTriggered = carActive;
        std::printf("[CAR] level init: loaded=%s active=%s scale=%.2f speed=%.2f\n",
            carModelLoaded ? "yes" : "NO", carActive ? "yes" : "no", carScale, carSpeed);
        std::printf("Уровень 'Перекрёсток' загружен\n");
    }

    keyW = false;
    keyA = false;
    keyS = false;
    keyD = false;
}

// ============================================================
// 11. ДИАЛОГ
// ============================================================

void startDialogue(const std::vector<DialogueLine>& lines,
    bool loadSchool = false)
{
    currentDialogue.lines = lines;
    currentDialogue.currentLine = 0;
    currentDialogue.loadSchoolAfter = loadSchool;
    currentDialogue.active = true;
    currentDialogue.waitingChoice = false;

    gamePhase = PHASE_DIALOGUE;
}

void triggerEnding(EndingType ending)
{
    currentEnding = ending;
    gameEnded = true;
    gamePhase = PHASE_DIALOGUE;

    std::string text;

    switch (ending) {
    case ENDING_BAD:
        if (currentLevel == LEVEL_LIBRARY) {
            text = "You chose to forget. The world returned to normal. "
                "But somewhere deep inside - emptiness.";
        }
        else {
            text = "You didn't read Futaba's notebook. "
                "You didn't understand her syndrome. "
                "And so - you forgot her. Forever.";
        }
        break;

    case ENDING_GOOD:
        text = "You remembered Mai. And she will remember you. "
            "The syndrome didn't win - because you didn't give up.";
        break;

    default:
        text = "";
        break;
    }

    currentDialogue.lines = {
        { "Ending", text, false,{} }
    };
    currentDialogue.currentLine = 0;
    currentDialogue.active = true;
    currentDialogue.waitingChoice = false;
    currentDialogue.loadSchoolAfter = false;
}

void advanceDialogue()
{
    if (!currentDialogue.active)
        return;

    if (currentDialogue.currentLine <
        static_cast<int>(currentDialogue.lines.size())) {

        const DialogueLine& line =
            currentDialogue.lines[currentDialogue.currentLine];

        if (line.isChoice) {
            currentDialogue.waitingChoice = true;
            return;
        }
    }

    currentDialogue.currentLine++;

    if (currentDialogue.currentLine >=
        static_cast<int>(currentDialogue.lines.size())) {

        currentDialogue.active = false;
        gamePhase = PHASE_PLAYING;

        if (pendingEnding != ENDING_NONE) {
            triggerEnding(pendingEnding);
            pendingEnding = ENDING_NONE;
            return;
        }

        if (currentDialogue.loadSchoolAfter) {
            loadLevel(LEVEL_SCHOOL);
            currentDialogue.loadSchoolAfter = false;
        }
    }
}

// ============================================================
// 12. МЫШЬ
// ============================================================

void mouseMove(int mx, int my)
{
    if (!mouseCaptured) return;

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    int centerX = w / 2;
    int centerY = h / 2;

    int dx = mx - centerX;
    int dy = my - centerY;

    if (dx == 0 && dy == 0) return;

    camera.rotate(dx * 0.2f, -dy * 0.2f);

    glutWarpPointer(centerX, centerY);
    glutPostRedisplay();
}

void mouseEnter(int state)
{
    if (state == GLUT_ENTERED) {
        mouseCaptured = true;
        glutSetCursor(GLUT_CURSOR_NONE);

        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);

        glutWarpPointer(w / 2, h / 2);
    }
    else {
        mouseCaptured = false;
        glutSetCursor(GLUT_CURSOR_INHERIT);
    }
}

void mouseClick(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        mouseCaptured = true;
        glutSetCursor(GLUT_CURSOR_NONE);

        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);

        glutWarpPointer(w / 2, h / 2);
    }
}

// ============================================================
// 13. UI — СЧЁТЧИК
// ============================================================

void renderUI()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH),
        glutGet(GLUT_WINDOW_HEIGHT), 0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);

    glColor3f(1, 1, 1);
    glRasterPos2f(20, 30);

    const char* text;

    if (currentLevel == LEVEL_EMPTY) {
        text = "Empty level. Press 1,2,4 to switch.";
    }
    else if (currentLevel == LEVEL_CROSSROAD) {
        if (carActive) {
            text = "RUN!";
        }
        else if (carTriggered) {
            text = "It's over...";
        }
        else {
            text = "Step on the road. Press 1,2,3 to switch.";
        }
    }
    else if (gameEnded) {
        text = "Game Over. Press ESC to exit.";
    }
    else if (npcCaught) {
        text = "Mai found!";
    }
    else if (currentLevel == LEVEL_SCHOOL) {
        if (!bookPicked) {
            text = "Find the book. Press E to read.";
        }
        else if (hasBookInfo) {
            text = "Mai is waiting. Press E to talk.";
        }
        else {
            text = "Mai is waiting...";
        }
    }
    else {
        text = "Find Mai. Press E to catch.";
    }

    for (const char* c = text; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    if (currentLevel == LEVEL_CROSSROAD && carDebug) {
        char line[256];
        const int h = glutGet(GLUT_WINDOW_HEIGHT);
        glColor3f(1.0f, 0.85f, 0.25f);
        std::snprintf(line, sizeof(line), "[CAR DEBUG] loaded=%s active=%s triggered=%s",
            carModelLoaded ? "yes" : "NO", carActive ? "yes" : "no",
            carTriggered ? "yes" : "no");
        glRasterPos2f(20, h - 70);
        for (const char* c = line; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
        std::snprintf(line, sizeof(line), "pos=(%.2f, %.2f, %.2f) angle=%.1f scale=%.2f speed=%.1f",
            carX, carY, carZ, carAngle, carScale, carSpeed);
        glRasterPos2f(20, h - 52);
        for (const char* c = line; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
        std::snprintf(line, sizeof(line), "player=(%.2f, %.2f, %.2f) | V: debug, R: reset, -/+: scale",
            camera.getX(), camera.getY(), camera.getZ());
        glRasterPos2f(20, h - 34);
        for (const char* c = line; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
    }

    glEnable(GL_FOG);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
// 14. UI — ДИАЛОГ
// ============================================================

void renderDialogue()
{
    if (!currentDialogue.active) return;

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0, 0, 0, 0.75f);

    glBegin(GL_QUADS);
    glVertex2f(50, h - 200);
    glVertex2f(w - 50, h - 200);
    glVertex2f(w - 50, h - 20);
    glVertex2f(50, h - 20);
    glEnd();

    glDisable(GL_BLEND);

    if (currentDialogue.waitingChoice) {
        glColor3f(0.8f, 0.9f, 1.0f);

        glRasterPos2f(70, h - 160);
        const char* c1 = "1. Forget her";
        for (const char* c = c1; *c; c++)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(70, h - 120);
        const char* c2 = "2. Remember her";
        for (const char* c = c2; *c; c++)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    else if (currentDialogue.currentLine <
        static_cast<int>(currentDialogue.lines.size())) {

        const DialogueLine& line =
            currentDialogue.lines[currentDialogue.currentLine];

        glColor3f(1.0f, 0.8f, 0.2f);
        glRasterPos2f(70, h - 170);

        for (char c : line.speaker)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

        glColor3f(1, 1, 1);
        glRasterPos2f(70, h - 120);

        for (char c : line.text)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    if (!currentDialogue.waitingChoice) {
        glColor3f(0.7f, 0.7f, 0.7f);
        glRasterPos2f(70, h - 40);
        const char* hint = gameEnded
            ? "Press ESC to exit"
            : "Press E to continue";
        for (const char* c = hint; *c; c++)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glEnable(GL_FOG);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
// 15. DISPLAY
// ============================================================

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    camera.apply();

    // ========================================================
    // ПУСТОЙ УРОВЕНЬ — один большой пол
    // ========================================================
    if (currentLevel == LEVEL_EMPTY) {
        float floorSize = 200.0f;
        float half = floorSize * 0.5f;

        static Texture* floorTex = TextureManager::instance().get("resources/grnd_zemlya.tga");

        if (floorTex) {
            glEnable(GL_TEXTURE_2D);
            floorTex->bind();
        }
        glColor3f(1, 1, 1);

        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0);     glVertex3f(-half, 0.0f, -half);
        glTexCoord2f(40, 0);    glVertex3f(half, 0.0f, -half);
        glTexCoord2f(40, 40);   glVertex3f(half, 0.0f, half);
        glTexCoord2f(0, 40);    glVertex3f(-half, 0.0f, half);
        glEnd();

        if (floorTex) {
            floorTex->unbind();
            glDisable(GL_TEXTURE_2D);
        }

        renderUI();
        renderDialogue();
        glutSwapBuffers();
        return;
    }

    // ========================================================
    // ПЕРЕКРЁСТОК — пол + дорога + машина, БЕЗ стен
    // ========================================================
    if (currentLevel == LEVEL_CROSSROAD) {
        float gs = lvl.gridSize;
        float roadY = 0.02f;

        static Texture* grassTex = TextureManager::instance().get("resources/grnd_zemlya.tga");
        static Texture* roadTex = TextureManager::instance().get("resources/asphalt.tga");

        // --- ПОЛ (трава) ---
        if (grassTex) {
            glEnable(GL_TEXTURE_2D);
            grassTex->bind();
        }
        glColor3f(1, 1, 1);

        for (size_t i = 0; i < lvl.cells.size(); i++) {
            for (size_t j = 0; j < lvl.cells[i].size(); j++) {
                float wx = i * gs + gs * 0.5f;
                float wz = j * gs + gs * 0.5f;
                float half = gs * 0.5f;

                glBegin(GL_QUADS);
                glNormal3f(0, 1, 0);
                glTexCoord2f(0, 0); glVertex3f(wx - half, 0.0f, wz - half);
                glTexCoord2f(1, 0); glVertex3f(wx + half, 0.0f, wz - half);
                glTexCoord2f(1, 1); glVertex3f(wx + half, 0.0f, wz + half);
                glTexCoord2f(0, 1); glVertex3f(wx - half, 0.0f, wz + half);
                glEnd();
            }
        }

        if (grassTex) {
            grassTex->unbind();
            glDisable(GL_TEXTURE_2D);
        }

        // --- ДОРОГА (асфальт) ---
        if (!lvl.roadCells.empty()) {
            if (roadTex) {
                glEnable(GL_TEXTURE_2D);
                roadTex->bind();
            }
            glColor3f(1, 1, 1);

            for (size_t i = 0; i < lvl.roadCells.size(); i++) {
                for (size_t j = 0; j < lvl.roadCells[i].size(); j++) {
                    if (lvl.roadCells[i][j] == '.') continue;

                    float wx = i * gs + gs * 0.5f;
                    float wz = j * gs + gs * 0.5f;
                    float half = gs * 0.5f;

                    glBegin(GL_QUADS);
                    glNormal3f(0, 1, 0);
                    glTexCoord2f(0, 0); glVertex3f(wx - half, roadY, wz - half);
                    glTexCoord2f(1, 0); glVertex3f(wx + half, roadY, wz - half);
                    glTexCoord2f(1, 1); glVertex3f(wx + half, roadY, wz + half);
                    glTexCoord2f(0, 1); glVertex3f(wx - half, roadY, wz + half);
                    glEnd();
                }
            }

            if (roadTex) {
                roadTex->unbind();
                glDisable(GL_TEXTURE_2D);
            }
        }

        // --- МАШИНА + диагностика ---
        // Не вызываем render() для не загруженной модели: это может скрыть
        // первопричину или привести к обращению к пустым данным.
        if (carModelLoaded && (carActive || carDebug)) {
            glm::mat4 projection(1.0f);
            glm::mat4 view(1.0f);
            glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projection));
            glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(view));

            glm::mat4 carModelMatrix = glm::translate(
                glm::mat4(1.0f), glm::vec3(carX, carY, carZ));
            carModelMatrix = glm::rotate(carModelMatrix,
                glm::radians(carAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            carModelMatrix = glm::scale(carModelMatrix,
                glm::vec3(carScale, carScale, carScale));

            const glm::mat4 carMVP = projection * view * carModelMatrix;
            carModel.render(carMVP);
        }

        renderUI();
        renderDialogue();
        glutSwapBuffers();
        return;
    }

    // ========================================================
    // ОБЫЧНЫЕ УРОВНИ (библиотека, школа)
    // ========================================================
    float gs = lvl.gridSize;
    float floorY = 0.0f;
    float ceilY = 2.0f;

    glm::mat4 projection;
    glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projection));

    glm::mat4 view;
    glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(view));

    // NPC
    glm::mat4 model = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(npcX, 0.0f, npcZ)
    );
    model = glm::rotate(
        model,
        glm::radians(npcAngle),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 mvp = projection * view * model;
    npcModel.render(mvp);

    if (book) book->render();

    for (auto& levelModel : levelModels) {
        if (levelModel) levelModel->render();
    }

    // --- ПОЛ ---
    static Texture* grassTex = TextureManager::instance().get("resources/grnd_zemlya.tga");
    static Texture* roadTex = TextureManager::instance().get("resources/asphalt.tga");

    if (grassTex) {
        glEnable(GL_TEXTURE_2D);
        grassTex->bind();
    }
    glColor3f(1, 1, 1);

    for (size_t i = 0; i < lvl.cells.size(); i++) {
        for (size_t j = 0; j < lvl.cells[i].size(); j++) {
            float wx = i * gs + gs * 0.5f;
            float wz = j * gs + gs * 0.5f;
            float half = gs * 0.5f;

            glBegin(GL_QUADS);
            glNormal3f(0, 1, 0);
            glTexCoord2f(0, 0); glVertex3f(wx - half, floorY, wz - half);
            glTexCoord2f(1, 0); glVertex3f(wx + half, floorY, wz - half);
            glTexCoord2f(1, 1); glVertex3f(wx + half, floorY, wz + half);
            glTexCoord2f(0, 1); glVertex3f(wx - half, floorY, wz + half);
            glEnd();
        }
    }

    if (grassTex) {
        grassTex->unbind();
        glDisable(GL_TEXTURE_2D);
    }

    // --- ДОРОГА ---
    if (!lvl.roadCells.empty()) {
        if (roadTex) {
            glEnable(GL_TEXTURE_2D);
            roadTex->bind();
        }
        glColor3f(1, 1, 1);

        for (size_t i = 0; i < lvl.roadCells.size(); i++) {
            for (size_t j = 0; j < lvl.roadCells[i].size(); j++) {
                if (lvl.roadCells[i][j] == '.') continue;

                float wx = i * gs + gs * 0.5f;
                float wz = j * gs + gs * 0.5f;
                float half = gs * 0.5f;

                glBegin(GL_QUADS);
                glNormal3f(0, 1, 0);
                glTexCoord2f(0, 0); glVertex3f(wx - half, 0.02f, wz - half);
                glTexCoord2f(1, 0); glVertex3f(wx + half, 0.02f, wz - half);
                glTexCoord2f(1, 1); glVertex3f(wx + half, 0.02f, wz + half);
                glTexCoord2f(0, 1); glVertex3f(wx - half, 0.02f, wz + half);
                glEnd();
            }
        }

        if (roadTex) {
            roadTex->unbind();
            glDisable(GL_TEXTURE_2D);
        }
    }

    // --- ПОТОЛОК И СТЕНЫ ---
    for (size_t i = 0; i < lvl.cells.size(); i++) {
        for (size_t j = 0; j < lvl.cells[i].size(); j++) {
            float wx = i * gs + gs * 0.5f;
            float wz = j * gs + gs * 0.5f;

            Plane ceiling(false, wx, ceilY, wz, gs, "resources/crete_stena_07.tga");
            ceiling.render();

            char code = lvl.cells[i][j];

            if (code == '.') continue;
            if (lvl.models.find(code) != lvl.models.end()) continue;

            auto it = lvl.objects.find(code);
            if (it == lvl.objects.end()) continue;

            std::string name = it->second;
            if (code == 'P') continue;

            std::string texPath = "resources/" + name + ".tga";

            WallType type = WallType::Full;
            if (name.find("_h") != std::string::npos)
                type = WallType::Horizontal;
            if (name.find("_v") != std::string::npos)
                type = WallType::Vertical;

            Wall wall(type, wx, 1.0f, wz, gs, texPath);
            wall.render();
        }
    }

    renderUI();
    renderDialogue();
    glutSwapBuffers();
}

// ============================================================
// 16. КЛАВИАТУРА
// ============================================================

void keyboardDown(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);

    if (gamePhase == PHASE_DIALOGUE) {

        if (currentDialogue.waitingChoice) {
            if (key == '1') {
                currentDialogue.waitingChoice = false;
                currentDialogue.active = false;
                gamePhase = PHASE_PLAYING;
                triggerEnding(ENDING_BAD);
            }
            else if (key == '2') {
                currentDialogue.waitingChoice = false;
                currentDialogue.active = false;
                gamePhase = PHASE_PLAYING;
                loadLevel(LEVEL_SCHOOL);
            }
            return;
        }

        if (key == 'e' || key == 'E' || key == ' ') {
            advanceDialogue();
        }
        return;
    }

    if (gameEnded) return;

    // Отладочное управление машиной на перекрёстке.
    if (key == 'v' || key == 'V') {
        carDebug = !carDebug;
        std::printf("[CAR] debug overlay: %s\n", carDebug ? "ON" : "OFF");
        glutPostRedisplay();
    }
    if ((key == 'r' || key == 'R') && currentLevel == LEVEL_CROSSROAD) {
        loadLevel(LEVEL_CROSSROAD);
        std::printf("[CAR] reset requested\n");
        glutPostRedisplay();
        return;
    }
    if ((key == '+' || key == '=') && currentLevel == LEVEL_CROSSROAD) {
        carScale *= 1.25f;
        std::printf("[CAR] scale=%.3f\n", carScale);
        glutPostRedisplay();
    }
    if ((key == '-' || key == '_') && currentLevel == LEVEL_CROSSROAD) {
        carScale /= 1.25f;
        if (carScale < 0.001f) carScale = 0.001f;
        std::printf("[CAR] scale=%.3f\n", carScale);
        glutPostRedisplay();
    }

    if (key == 'w' || key == 'W') keyW = true;
    if (key == 's' || key == 'S') keyS = true;
    if (key == 'a' || key == 'A') keyA = true;
    if (key == 'd' || key == 'D') keyD = true;

    if (key == '1') loadLevel(LEVEL_LIBRARY);
    if (key == '2') loadLevel(LEVEL_SCHOOL);
    if (key == '3') loadLevel(LEVEL_EMPTY);
    if (key == '4') loadLevel(LEVEL_CROSSROAD);

    if (key == 'e' || key == 'E') {

        if (book && !bookPicked && currentLevel == LEVEL_SCHOOL) {
            float dx = camera.getX() - bookX;
            float dz = camera.getZ() - bookZ;

            if (sqrtf(dx * dx + dz * dz) < 1.5f) {
                bookPicked = true;
                hasBookInfo = true;

                delete book;
                book = nullptr;

                startDialogue({
                    { "Rio Futaba", "This is my research on the 'Sakurajima syndrome'...", false,{} },
                    { "Rio Futaba", "Mai pushes people away because she's afraid.", false,{} },
                    { "Rio Futaba", "But deep down, she wants to be close to someone.", false,{} },
                    { "Rio Futaba", "If you want to help her - don't chase her.", false,{} },
                    { "Rio Futaba", "Wait. Be patient. Let her come to you.", false,{} }
                    }, false);
                return;
            }
        }

        if (npcCaught) return;

        float px = camera.getX();
        float pz = camera.getZ();

        float dx = npcX - px;
        float dz = npcZ - pz;

        float distance = sqrtf(dx * dx + dz * dz);

        if (distance <= npcCatchDistance) {
            npcCaught = true;
            npcWasCaught = false;
            npcWaiting = false;

            if (currentLevel == LEVEL_LIBRARY) {
                std::vector<DialogueLine> lines = {
                    { "Mai", "Stop with that 'Senpai' thing.", false,{} },
                    { "Mai", "You really put in a great effort.", false,{} },
                    { "Mai", "But... forget what you saw today.", false,{} },
                    { "Mai", "It's better for both of us.", false,{} },
                    { "", "Choice", true,{
                        { "1. Forget her", 0 },
                        { "2. Remember her", 1 }
                    } }
                };
                startDialogue(lines, false);
            }
            else {
                if (hasBookInfo) {
                    std::vector<DialogueLine> lines = {
                        { "Mai", "Stop with that 'Senpai' thing.", false,{} },
                        { "Mai", "You really put in a great effort.", false,{} },
                        { "Mai", "But... forget what you saw today.", false,{} },
                        { "Mai", "It's better for both of us.", false,{} },
                        { "You", "No. I won't forget.", false,{} },
                        { "You", "I read Futaba's notebook.", false,{} },
                        { "You", "I know about your syndrome.", false,{} },
                        { "Mai", "...", false,{} },
                        { "Mai", "You... you know?", false,{} },
                        { "Mai", "Then... maybe I don't have to run.", false,{} }
                    };
                    startDialogue(lines, false);
                    pendingEnding = ENDING_GOOD;
                }
                else {
                    std::vector<DialogueLine> lines = {
                        { "Mai", "Stop with that 'Senpai' thing.", false,{} },
                        { "Mai", "You really put in a great effort.", false,{} },
                        { "Mai", "But... forget what you saw today.", false,{} },
                        { "Mai", "It's better for both of us.", false,{} },
                        { "You", "...", false,{} },
                        { "You", "I don't understand...", false,{} },
                        { "Mai", "That's why you should forget.", false,{} }
                    };
                    startDialogue(lines, false);
                    pendingEnding = ENDING_BAD;
                }
            }
        }
    }
}

void keyboardUp(unsigned char key, int x, int y)
{
    if (key == 'w' || key == 'W') keyW = false;
    if (key == 's' || key == 'S') keyS = false;
    if (key == 'a' || key == 'A') keyA = false;
    if (key == 'd' || key == 'D') keyD = false;
}

// ============================================================
// 17. UPDATE
// ============================================================

void update(int value)
{
    float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt = now - lastTime;

    lastTime = now;

    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;

    if (gamePhase == PHASE_PLAYING && !gameEnded) {
        const float speed = 0.1f;
        float dx = 0.0f;
        float dz = 0.0f;

        if (keyW) {
            float fdx, fdz;
            camera.getForwardDelta(speed, fdx, fdz);
            dx += fdx; dz += fdz;
        }
        if (keyS) {
            float fdx, fdz;
            camera.getForwardDelta(-speed, fdx, fdz);
            dx += fdx; dz += fdz;
        }
        if (keyA) {
            float rdx, rdz;
            camera.getRightDelta(-speed, rdx, rdz);
            dx += rdx; dz += rdz;
        }
        if (keyD) {
            float rdx, rdz;
            camera.getRightDelta(speed, rdx, rdz);
            dx += rdx; dz += rdz;
        }

        float len = sqrtf(dx * dx + dz * dz);
        if (len > speed) {
            dx = dx / len * speed;
            dz = dz / len * speed;
        }

        if (currentLevel == LEVEL_EMPTY ||
            currentLevel == LEVEL_CROSSROAD) {
            camera.setPosition(camera.getX() + dx,
                camera.getY(),
                camera.getZ() + dz);
        }
        else {
            camera.tryMove(dx, dz, lvl, 0.3f);
        }
    }

    npcModel.update(dt);

    // ========================================================
    // МАШИНА
    // ========================================================
    if (currentLevel == LEVEL_CROSSROAD) {
        // Машина активна сразу после загрузки уровня. Триггер больше не
        // зависит от того, попал ли игрок точно в клетку дороги.
        if (carModelLoaded && !carActive && !carTriggered) {
            carActive = true;
            carTriggered = true;
            std::printf("[CAR] auto-start at (%.2f, %.2f, %.2f)\n", carX, carY, carZ);
        }

        if (carModelLoaded && carActive) {
            // Двигаем машину.
            carZ += carSpeed * dt;

            // Границы карты в мировых координатах.
            const float gs = lvl.gridSize;

            const float mapMinX = 0.0f;
            const float mapMinZ = 0.0f;

            const float mapMaxX = static_cast<float>(lvl.cells.size()) * gs;

            float mapMaxZ = 0.0f;

            for (const auto& row : lvl.cells) {
                if (row.size() > mapMaxZ / gs) {
                    mapMaxZ = static_cast<float>(row.size()) * gs;
                }
            }

            // Если машина покинула карту — возвращаем её на спавн.
           // Если машина покинула карту — возвращаем её
// в ту же мировую позицию, в которой она появилась.
            if (carX < mapMinX || carX >= mapMaxX ||
                carZ < mapMinZ || carZ >= mapMaxZ) {

                const float gs = lvl.gridSize;

                if (lvl.carSpawnI >= 0 && lvl.carSpawnJ >= 0) {
                    carX = (lvl.carSpawnI + 0.5f) * gs;
                    carZ = (lvl.carSpawnJ + 0.5f) * gs;
                }

                carY = 0.08f;
                carAngle = 0.0f;
                carDebugLogTimer = 0.0f;

                std::printf(
                    "[CAR] Машина вернулась на спавн: "
                    "(%.2f, %.2f, %.2f)\n",
                    carX, carY, carZ
                );
            }

            const float dx = camera.getX() - carX;
            const float dz = camera.getZ() - carZ;
            const float dist = sqrtf(dx * dx + dz * dz);

            // Логируем позицию не каждый кадр, а примерно раз в секунду.
            carDebugLogTimer += dt;
            if (carDebug && carDebugLogTimer >= 1.0f) {
                carDebugLogTimer = 0.0f;
                std::printf("[CAR][TICK] pos=(%.2f, %.2f, %.2f), player=(%.2f, %.2f, %.2f), dist=%.2f, scale=%.2f\n",
                    carX, carY, carZ, camera.getX(), camera.getY(), camera.getZ(), dist, carScale);
            }

            if (dist < 1.5f && gamePhase == PHASE_PLAYING) {
                std::printf("[CAR] столкновение с игроком! distance=%.2f\n", dist);
                carActive = false;
                startDialogue({
                    { "Narrator", "The car hit you. Everything went dark...", false,{} },
                    { "Narrator", "You never had the chance to save her.", false,{} },
                    { "Narrator", "And so - you forgot Mai. Forever.", false,{} }
                    }, false);
                pendingEnding = ENDING_BAD;
            }
        }
    }

    if (currentLevel == LEVEL_EMPTY ||
        currentLevel == LEVEL_CROSSROAD) {
        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }

    if (npcWaiting) {
        float playerX = camera.getX();
        float playerZ = camera.getZ();
        float dx = playerX - npcX;
        float dz = playerZ - npcZ;
        float dist = sqrtf(dx * dx + dz * dz);

        if (dist < 5.0f) {
            float targetAngle = atan2f(dx, dz) * 180.0f / 3.14159265f;
            float diff = targetAngle - npcAngle;
            while (diff > 180.0f)  diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
            npcAngle += diff * 0.05f;
        }
    }
    else if (!npcCaught && !gameEnded) {
        if (npcWasCaught) {
            npcModel.setAnimation(NPC_WALK_ANIMATION);
            npcWasCaught = false;
        }

        if (!npcPaths.empty() &&
            currentPath >= 0 &&
            currentPath < static_cast<int>(npcPaths.size())) {

            const auto& path = npcPaths[currentPath];

            if (path.size() >= 2 &&
                currentWaypoint >= 0 &&
                currentWaypoint < static_cast<int>(path.size())) {

                Waypoint target = path[currentWaypoint];

                float dx = target.x - npcX;
                float dz = target.z - npcZ;
                float dist = sqrtf(dx * dx + dz * dz);

                if (dist > 0.01f) {
                    float targetAngle = atan2f(dx, dz) * 180.0f / 3.14159265f;
                    float diff = targetAngle - npcAngle;

                    while (diff > 180.0f)  diff -= 360.0f;
                    while (diff < -180.0f) diff += 360.0f;

                    npcAngle += diff * 0.1f;
                }

                if (dist > 0.1f) {
                    float step = npcSpeed * dt;
                    if (step > dist) step = dist;

                    npcX += (dx / dist) * step;
                    npcZ += (dz / dist) * step;
                }
                else {
                    if (npcForward) {
                        currentWaypoint++;
                        if (currentWaypoint >= static_cast<int>(path.size())) {
                            chooseNextPatrolPath();
                        }
                    }
                    else {
                        currentWaypoint--;
                        if (currentWaypoint < 0) {
                            chooseNextPatrolPath();
                        }
                    }
                }
            }
        }
    }
    else if (npcCaught && !gameEnded) {
        if (!npcWasCaught) {
            npcModel.setAnimation(NPC_CAUGHT_ANIMATION);
            npcWasCaught = true;
        }

        float playerX = camera.getX();
        float playerZ = camera.getZ();

        float dx = playerX - npcX;
        float dz = playerZ - npcZ;

        float targetAngle = atan2f(dx, dz) * 180.0f / 3.14159265f;
        float diff = targetAngle - npcAngle;

        while (diff > 180.0f)  diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;

        npcAngle += diff * 0.15f;
    }

    frameCount++;
    fpsTimer += dt;

    if (fpsTimer >= 1.0f) {
        char title[64];
        snprintf(title, sizeof(title), "mxEngine - FPS: %d", frameCount);
        glutSetWindowTitle(title);
        frameCount = 0;
        fpsTimer = 0.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// ============================================================
// 18. INIT / RESHAPE
// ============================================================

void init()
{
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.5f, 0.6f, 0.7f, 1.0f);

    GLfloat fogColor[] = { 0.5f, 0.6f, 0.7f, 1.0f };
    glEnable(GL_FOG);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.08f);
    glHint(GL_FOG_HINT, GL_NICEST);

    std::printf("Туман включён\n");
}

void reshape(int width, int height)
{
    if (height == 0) height = 1;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0f,
        static_cast<float>(width) / static_cast<float>(height),
        0.1f, 500.0f);

    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
// 19. MAIN
// ============================================================

int main(int argc, char** argv)
{
    std::setlocale(LC_ALL, "ru_RU.UTF-8");

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    srand(static_cast<unsigned int>(time(nullptr)));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("mxEngine");

    glewExperimental = GL_TRUE;

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "GLEW init failed: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    std::cout << "GLEW OK, version: " << glewGetString(GLEW_VERSION) << std::endl;
    char workingDir[MAX_PATH] = {};
    if (GetCurrentDirectoryA(MAX_PATH, workingDir))
        std::printf("[DEBUG] working directory: %s\n", workingDir);

    // NPC
    tinygltf::Model rawModel;
    if (!GLBLoader::load("resources/retargeted_animations.glb", rawModel)) {
        std::cerr << "Ошибка загрузки GLB-модели NPC" << std::endl;
        return -1;
    }
    npcModel.load(rawModel);

    // Машина: подробная диагностика загрузки GLB.
    tinygltf::Model carRawModel;
    const char* carPath = "resources/source/car.glb";
    std::printf("[CAR][LOAD] trying: %s\n", carPath);
    if (GLBLoader::load(carPath, carRawModel)) {
        std::printf("[CAR][LOAD] GLBLoader OK: nodes=%zu meshes=%zu materials=%zu textures=%zu scenes=%zu animations=%zu\n",
            carRawModel.nodes.size(), carRawModel.meshes.size(),
            carRawModel.materials.size(), carRawModel.textures.size(),
            carRawModel.scenes.size(), carRawModel.animations.size());
        carModel.load(carRawModel);
        carModelLoaded = !carRawModel.meshes.empty() || !carRawModel.nodes.empty();
        std::printf("[CAR][LOAD] carModel.load() called; usable model=%s\n",
            carModelLoaded ? "yes" : "NO (GLB has no nodes/meshes)");
        if (!carModelLoaded)
            std::cerr << "[CAR][ERROR] GLB загружен, но в нём нет nodes/meshes." << std::endl;
    }
    else {
        carModelLoaded = false;
        std::cerr << "[CAR][ERROR] Не удалось загрузить " << carPath
            << ". Проверь рабочую папку запуска и существование файла." << std::endl;
    }

    loadLevel(LEVEL_CROSSROAD);

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutReshapeFunc(reshape);

    glutPassiveMotionFunc(mouseMove);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseClick);
    glutEntryFunc(mouseEnter);

    lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glutTimerFunc(16, update, 0);

    glutMainLoop();

    return 0;
}