
#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>   // ВСЕГДА ПЕРВЫМ!
#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include "ModelLoader.h"
#include "Cube.h"
#include "Wall.h"

#include "Terrain.h"
#include "Camera.h"
#include "Map.h"
#include "Plane.h"
#include "GLBModel.h"
#include "GLBLoader.h"
using namespace std;
float cameraX = 4.0f;
float cameraY = 1.0f;
float cameraZ = 10.0f;


int sectorX = 0;
int sectorY = 0;
int sectorZ = 0;

Model mdl(2, 1, 2);

GLBModel npcModel;
Camera camera;
bool mouseCaptured = false;

Level lvl;

struct Waypoint {
    float x, z;
};

std::vector<std::vector<Waypoint>> npcPaths = {
    {
        { 3.0f,  7.0f },
        { 3.0f, 13.0f }
    },
    // Коридор 1
    {
        { 7.0f,  7.0f },
        { 7.0f, 13.0f }
    },

    // Коридор 2
    {
        { 11.0f, 7.0f },
        { 11.0f, 13.0f }
    }
};

int currentPath = 0;
int currentWaypoint = 1;

bool npcForward = true;

float npcSpeed = 2.0f;
float npcX = 0.0f;
float npcY = 0.0f;
float npcZ = 0.0f;
float npcAngle = 0.0f;

// Глобально, рядом с camera и lvl:
float lastTime = 0.0f;
int frameCount = 0;
float fpsTimer = 0.0f;

bool npcWasCaught = false;
void mouseMove(int mx, int my) {
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

void mouseEnter(int state) {
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

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        mouseCaptured = true;
        glutSetCursor(GLUT_CURSOR_NONE);
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        glutWarpPointer(w / 2, h / 2);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    camera.apply();

    float gs = lvl.gridSize;
    float floorY = 0.0f;
    float ceilY = 2.0f;
    glm::mat4 projection;
    glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projection));

    glm::mat4 view;
    glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(view)); // это view * model — но для камеры это view
    glm::mat4 model = glm::translate(glm::mat4(1.0f),
        glm::vec3(npcX, 0.0f, npcZ));
    model = glm::rotate(model, glm::radians(npcAngle),
        glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 mvp = projection * view * model;

    npcModel.render(mvp);
    for (size_t i = 0; i < lvl.cells.size(); i++) {
        for (size_t j = 0; j < lvl.cells[i].size(); j++) {
            float wx = i * gs + gs * 0.5f;
            float wz = j * gs + gs * 0.5f;

            // Пол и потолок — для всех клеток
            Plane floor(true, wx, floorY, wz, gs, "resources/crete_stena_07.tga");
            floor.render();
            Plane ceiling(false, wx, ceilY, wz, gs, "resources/crete_stena_07.tga");
            ceiling.render();

            // Стены
            char code = lvl.cells[i][j];
            if (code == '.') continue;

            auto it = lvl.objects.find(code);
            if (it == lvl.objects.end()) continue;

            std::string name = it->second;
            std::string texPath = "resources/" + name + ".tga";

            // Определяем тип стены по имени
            WallType type = WallType::Full;
            if (name.find("_h") != std::string::npos) type = WallType::Horizontal;
            if (name.find("_v") != std::string::npos) type = WallType::Vertical;

            Wall wall(type, wx, 1.0f, wz, gs, texPath);
            wall.render();
        }
    }
    // Рисуем NPC в нужной позиции
    glPushMatrix();
    glTranslatef((lvl.spawnI + 0.5f) * gs, 0.0f, (lvl.spawnJ + 0.5f) * gs);
    //glRotatef(npcAngle, 0.0f, 1.0f, 0.0f);

    glPopMatrix();
    glutSwapBuffers();
}

bool keyW = false, keyA = false, keyS = false, keyD = false;
bool npcCaught = false;
float npcCatchDistance = 1.5f;

void keyboardDown(unsigned char key, int x, int y) {
    if (key == 'w') keyW = true;
    if (key == 's') keyS = true;
    if (key == 'a') keyA = true;
    if (key == 'd') keyD = true;

    if (key == 'e') {
        // Если NPC уже пойман — ничего не делаем
        if (npcCaught)
            return;

        float px = camera.getX();
        float pz = camera.getZ();

        float dx = npcX - px;
        float dz = npcZ - pz;

        float distance = sqrtf(dx * dx + dz * dz);

        if (distance <= npcCatchDistance) {
            npcCaught = true;

            std::cout << "NPC POIMAN!" << std::endl;
        }
    }

    if (key == 27)
        exit(0);
}

void keyboardUp(unsigned char key, int x, int y) {
    if (key == 'w') keyW = false;
    if (key == 's') keyS = false;
    if (key == 'a') keyA = false;
    if (key == 'd') keyD = false;
}
void teleportNPCToPath(int pathIndex, bool fromStart)
{
    currentPath = pathIndex;

    const auto& path = npcPaths[currentPath];

    if (path.size() < 2)
        return;

    if (fromStart) {
        // Начинаем с первой точки и идём ко второй
        currentWaypoint = 1;
        npcForward = true;

        npcX = path[0].x;
        npcZ = path[0].z;
    }
    else {
        // Начинаем со второй точки и идём к первой
        currentWaypoint = 0;
        npcForward = false;

        npcX = path[1].x;
        npcZ = path[1].z;
    }
}
void update(int value) {
    const float speed = 0.1f;
    float dx = 0, dz = 0;

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

    camera.tryMove(dx, dz, lvl, 0.3f);

    // --- Тайминг ---
    float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt = now - lastTime;
    lastTime = now;

    // Защита от огромного dt (например, после паузы)
    if (dt > 0.1f) dt = 0.1f;

    npcModel.update(dt);
    // --- Патруль NPC ---
    npcModel.update(dt);

    // ==========================================
    // NPC ПАТРУЛИРУЕТ
    // ==========================================
    if (!npcCaught) {

        // Если NPC только что вернулся из состояния пойманного
        if (npcWasCaught) {
            npcModel.setAnimation(1);
            npcWasCaught = false;
        }

        // -------------------------------
        // ПАТРУЛЬ
        // -------------------------------

        if (!npcPaths.empty()) {

            auto& path = npcPaths[currentPath];

            if (path.size() >= 2) {

                Waypoint target = path[currentWaypoint];

                float dx = target.x - npcX;
                float dz = target.z - npcZ;
                float dist = sqrtf(dx * dx + dz * dz);

                // Поворот к цели
                if (dist > 0.01f) {

                    float targetAngle =
                        atan2f(dx, dz) * 180.0f / 3.14159265f;

                    float diff = targetAngle - npcAngle;

                    while (diff > 180.0f)
                        diff -= 360.0f;

                    while (diff < -180.0f)
                        diff += 360.0f;

                    npcAngle += diff * 0.1f;
                }

                // Движение
                if (dist > 0.1f) {

                    float step = npcSpeed * dt;

                    if (step > dist)
                        step = dist;

                    npcX += (dx / dist) * step;
                    npcZ += (dz / dist) * step;
                }
                else {

                    // ==========================================
                    // КОНЕЦ ТЕКУЩЕГО НАПРАВЛЕНИЯ
                    // ==========================================

                    if (npcForward) {

                        currentWaypoint++;

                        if (currentWaypoint >= (int)path.size()) {

                            // Случайный другой коридор
                            int newPath;

                            if (npcPaths.size() > 1) {

                                do {
                                    newPath = rand() % npcPaths.size();
                                } while (newPath == currentPath);

                            }
                            else {
                                newPath = currentPath;
                            }

                            currentPath = newPath;

                            const auto& newPathData =
                                npcPaths[currentPath];

                            // Случайный конец коридора
                            if (rand() % 2 == 0) {

                                npcX = newPathData[0].x;
                                npcZ = newPathData[0].z;

                                currentWaypoint = 1;
                                npcForward = true;

                            }
                            else {

                                npcX = newPathData[1].x;
                                npcZ = newPathData[1].z;

                                currentWaypoint = 0;
                                npcForward = false;
                            }
                        }
                    }
                    else {

                        currentWaypoint--;

                        if (currentWaypoint < 0) {

                            // Случайный другой коридор
                            int newPath;

                            if (npcPaths.size() > 1) {

                                do {
                                    newPath = rand() % npcPaths.size();
                                } while (newPath == currentPath);

                            }
                            else {
                                newPath = currentPath;
                            }

                            currentPath = newPath;

                            const auto& newPathData =
                                npcPaths[currentPath];

                            // Случайный конец коридора
                            if (rand() % 2 == 0) {

                                npcX = newPathData[0].x;
                                npcZ = newPathData[0].z;

                                currentWaypoint = 1;
                                npcForward = true;

                            }
                            else {

                                npcX = newPathData[1].x;
                                npcZ = newPathData[1].z;

                                currentWaypoint = 0;
                                npcForward = false;
                            }
                        }
                    }
                }
            }
        }
    }

    // ==========================================
    // NPC ПОЙМАН
    // ==========================================
    else {

        if (!npcWasCaught) {

            // Переключаемся на анимацию
            // "рука на бок"
            npcModel.setAnimation(0);

            npcWasCaught = true;
        }

        // ======================================
        // ПОВОРАЧИВАЕМ NPC К ИГРОКУ
        // ======================================

        float playerX = camera.getX();
        float playerZ = camera.getZ();

        float dx = playerX - npcX;
        float dz = playerZ - npcZ;

        float targetAngle =
            atan2f(dx, dz) * 180.0f / 3.14159265f;

        float diff = targetAngle - npcAngle;

        while (diff > 180.0f)
            diff -= 360.0f;

        while (diff < -180.0f)
            diff += 360.0f;

        // Плавный разворот к игроку
        npcAngle += diff * 0.15f;
    }


    // --- Счётчик FPS ---
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

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0); // Устанавливаем цвет фона в черный
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}



int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "ru_RU.UTF-8"); // или просто "" для системной локали
#include <windows.h>
    SetConsoleOutputCP(CP_UTF8); // Для вывода
    SetConsoleCP(CP_UTF8);        // Для ввода
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    
    glutCreateWindow("Simple OpenGL Camera with GLUT ");

    // Инициализация GLEW
    glewExperimental = GL_TRUE; // Важно для современного OpenGL [citation:4][citation:8]
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "GLEW init failed: " << glewGetErrorString(err) << std::endl;
        return -1;
    }
    std::cout << "GLEW OK, version: " << glewGetString(GLEW_VERSION) << std::endl;

    /*if (!loadOBJ("electropole.obj")) {
        return -1; // Ошибка загрузки модели
    }*/
    /*if (!mdl.loadOBJ("resources/ak19.obj")) {
        return -1; // Ошибка загрузки модели
    }*/


    // ... в display() или в main():
    //GLBModel npcModel;

    // В main() после glutCreateWindow:

    tinygltf::Model rawModel;
    if (GLBLoader::load("resources/retargeted_animations.glb", rawModel)) {
        npcModel.load(rawModel);
    }
    npcModel.setAnimation(1);  // Walk_RT
    /*npcX = npcPath[0].x;
    npcZ = npcPath[0].z;
    */
    teleportNPCToPath(0, true);
    if (!loadMap("resources/maze01.map", lvl)) {
        std::cerr << "Ошибка загрузки карты" << std::endl;
        exit(1);
    }

    if (lvl.spawnI >= 0 && lvl.spawnJ >= 0) {
        float gs = lvl.gridSize;
        camera.setPosition((lvl.spawnI + 0.5f) * gs, 1.4f, (lvl.spawnJ + 0.5f) * gs);
        camera.setRotation(0.0f, 0.0f);
    }

    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutTimerFunc(16, update, 0);
    glutReshapeFunc(reshape);

    glutPassiveMotionFunc(mouseMove);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseClick);
    glutEntryFunc(mouseEnter);

    glutMainLoop();
    return 0;
}
