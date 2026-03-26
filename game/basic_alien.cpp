#include "basic_alien.hpp"
#include <cstdlib>

BasicAlien::BasicAlien() : Alien(30, 3, 32, 5, BASIC) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    moveCounter = 0;
}

BasicAlien::BasicAlien(int x_in, int y_in) : Alien(30, 3, x_in, y_in, BASIC) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    moveCounter = 0;
}

BasicAlien::~BasicAlien() {
}

void BasicAlien::move(int dx, int dy) {
    moveCounter++;

    // Random direction changes every 15-20 frames
    if (moveCounter % 18 == 0) {
        if (rand() % 2 == 0) {
            horizontalDirection *= -1;
        }
    }

    // Bounce off edges
    int currentX = get_x();
    if (currentX <= 2 || currentX >= 61) {
        horizontalDirection *= -1;
    }

    Alien::move(horizontalDirection, dy);
}

void BasicAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;  // Completely dead, don't draw

    int x = get_x();
    int y = get_y();

    // Bounds check - skip if completely off screen (allow partial at bottom)
    if (x < 1 || x >= 63 || y < 1 || y > 66) return;

    if (get_health() == 0) {
        Alien::draw(matrix);
        return;
    }

    // Safe pixel setter - clips to screen bounds, flashes white when hit
    int flash = getHitFlash();
    auto sp = [matrix, flash](int py, int px, int r, int g, int b) {
        if (px >= 0 && px < 64 && py >= 0 && py < 64) {
            if (flash > 0) {
                int f = flash * 50;
                r = r + (160 - r) * f / 200;
                g = g + (30 - g) * f / 200;
                b = b + (30 - b) * f / 200;
            }
            matrix->SetPixel(py, px, r, g, b);
        }
    };

    // Muted sage green small X-shaped alien with short tail
    sp(y, x, 80, 150, 90);
    sp(y + 1, x - 1, 70, 135, 80);
    sp(y + 1, x + 1, 70, 135, 80);
    sp(y - 1, x - 1, 50, 105, 60);
    sp(y - 1, x + 1, 50, 105, 60);
    sp(y - 1, x, 40, 90, 50);

    // Flashing single engine dot at the back (top) - dim magenta
    if ((moveCounter / 2) % 2 == 0) {
        sp(y - 2, x, 100, 25, 90);
    } else {
        sp(y - 2, x, 35, 10, 30);
    }
}
