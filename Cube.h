
#include <GL/glut.h>
class Cube {
public:
    Cube(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    void render();
private:
    int x, y, z;
};