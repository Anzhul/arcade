#include "bullet.hpp"

Bullet::Bullet() : x(0), y(0), dx(0), dy(-1), r(255), g(0), b(0), active(false) {}

Bullet::Bullet(int x, int y, int dx, int dy, int r, int g, int b)
    : x(x), y(y), dx(dx), dy(dy), r(r), g(g), b(b), active(true) {}

void Bullet::update() {
    if (!active) return;

    x += dx;
    y += dy;

    // Deactivate if off screen
    if (x < 0 || x >= 64 || y < 0 || y >= 64) {
        active = false;
    }
}

void Bullet::draw(RGBMatrix *matrix) {
    if (!active) return;
    matrix->SetPixel(y, x, r, g, b);
}

void Bullet::erase(RGBMatrix *matrix) {
    if (!active) return;
    matrix->SetPixel(y, x, 18, 10, 14);  // Background color
}
