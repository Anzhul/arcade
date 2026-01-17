#ifndef BULLET_HPP
#define BULLET_HPP

#include "led-matrix.h"
using namespace rgb_matrix;

class Bullet {
public:
    Bullet();
    Bullet(int x, int y, int dx, int dy, int r, int g, int b);

    void update();
    void draw(RGBMatrix *matrix);
    void erase(RGBMatrix *matrix);

    bool isActive() const { return active; }
    void deactivate() { active = false; }

    int getX() const { return x; }
    int getY() const { return y; }

private:
    int x, y;       // Position
    int dx, dy;     // Direction/velocity
    int r, g, b;    // Color
    bool active;
};

#endif
