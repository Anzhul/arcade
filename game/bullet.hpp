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

    void setLarge(bool large) { isLarge = large; }
    bool getLarge() const { return isLarge; }
    
    void setRocket(bool rocket) { isRocket = rocket; }
    bool getRocket() const { return isRocket; }
    
    void setScatter(bool scatter) { isScatter = scatter; }
    bool getScatter() const { return isScatter; }

    void setHoming(bool h) { isHoming = h; }
    bool getHoming() const { return isHoming; }
    void setMissile(bool m) { isMissile = m; }
    bool getMissile() const { return isMissile; }
    int getTick() const { return tickCount; }
    void setDx(int newDx) { dx = newDx; }
    void setDy(int newDy) { dy = newDy; }
    int getDx() const { return dx; }
    int getDy() const { return dy; }

private:
    int x, y;       // Position
    int dx, dy;     // Direction/velocity
    int r, g, b;    // Color
    bool active;
    bool isLarge;   // Large bullet (2x2)
    bool isRocket;  // Rocket (explodes on impact)
    bool isScatter; // Scatter shot bullet
    bool isHoming;  // Slow homing missile
    bool isMissile; // Slow blinking launcher missile
    int tickCount;  // Frame counter for blinking
};

#endif
