
#include <GL/glut.h>
class Terrain {
public:
    Terrain(/*int x, int y, int z*/) {
        //obj_x = x;
        //obj_y = y;
        //obj_z = z;
    }
    void render();
private:
    //int obj_x, obj_y, obj_z;
    /*int HeightMap[5][5] = {
        1, 2, 2, 2 ,3,
        1, 1 ,2, 2, 2,
        0, 1, 2, 2, 1,
        0, 1, 1, 1, 1,
        0, 0, 0, 0, 0
    };*/
    int HeightMap[5][5] = {
       0, 0, 0, 0 ,0,
       0, 0 ,0, 0, 0,
       0, 0, 0, 0, 0,
       0, 0, 0, 0, 0,
       0, 0, 0, 0, 0
   };
};