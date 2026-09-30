#include "Terrain.h"
void Terrain::render() {
    const unsigned int MapSize = 5; // Размер карты
    int Zoom = 2; // Увеличение для ширины ландшафта
    int x, y;

    for (int i = 0; i < MapSize - 1; i++) {
        for (int j = 0; j < MapSize - 1; j++) {
            x = i * Zoom;
            y = j * Zoom;

            glBegin(GL_TRIANGLE_STRIP);
            glColor3f(0.0f, 1.0f, 0.0f); // Задаем цвет для ландшафта

            // Здесь Z - высота
            glVertex3f(x, HeightMap[i][j], y);         // Низкий левый угол
            glVertex3f(x + Zoom, HeightMap[i + 1][j], y); // Низкий правый угол
            glVertex3f(x, HeightMap[i][j + 1], y + Zoom); // Верхний левый угол
            glVertex3f(x + Zoom, HeightMap[i + 1][j + 1], y + Zoom); // Верхний правый угол

            glEnd();
        }
    }
}