#include "tank_alien.hpp"
#include <algorithm>  // for std::max, std::min

TankAlien::TankAlien() : Alien(60, 1, 32, 5, TANK) {
    setX(std::max(4, std::min(59, get_x())));
    setY(std::max(4, std::min(59, get_y())));
    frameCounter = 0;
    behaviorMode = 0;
    horizontalDirection = (rand() % 2) * 2 - 1;
    moveCounter = 0;
    anchorY = 16 + (rand() % 10);  // Guard anchors at y=16-26
    anchored = false;
}

TankAlien::TankAlien(int x_in, int y_in) : Alien(60, 1, x_in, y_in, TANK) {
    setX(std::max(4, std::min(59, x_in)));
    setY(std::max(4, std::min(59, y_in)));
    frameCounter = 0;
    behaviorMode = 0;
    horizontalDirection = (rand() % 2) * 2 - 1;
    moveCounter = 0;
    anchorY = 16 + (rand() % 10);
    anchored = false;
}

TankAlien::~TankAlien() {
}

void TankAlien::move(int dx, int dy) {
    moveCounter++;

    int currentX = get_x();
    if (currentX <= 6 || currentX >= 57) {
        horizontalDirection *= -1;
    }

    if (behaviorMode == 0) {
        // DEFAULT: straight down
        Alien::move(0, dy);
    } else if (behaviorMode == 1) {
        // GUARD: move right with carousel wrap, descend until anchor then hold level
        int descend = 0;
        if (!anchored) {
            if (get_y() < anchorY) {
                descend = (moveCounter % 2 == 0) ? 1 : 0;
            } else {
                anchored = true;
            }
        }
        Alien::move(1, descend);
        // Wrap when fully off right side
        if (get_x() - 4 > 63) {
            setX(-8);
        }
    } else if (behaviorMode == 2) {
        // PATROL: zigzag left-right while descending
        if (moveCounter % 15 == 0) {
            horizontalDirection *= -1;
        }
        Alien::move(horizontalDirection, dy);
    } else if (behaviorMode == 3) {
        // CAROUSEL: descend to anchor, then move right slowly descending, wrap around
        if (!anchored) {
            if (get_y() < anchorY) {
                Alien::move(0, dy);
            } else {
                anchored = true;
            }
            return;
        }
        // Descent while moving right
        int descend = (moveCounter % 2 == 0) ? 1 : 0;
        Alien::move(1, descend);
        // Wrap when right edge (x+4) is fully off the right side
        if (get_x() - 4 > 63) {
            setX(-8);  // Start fully off left side, right edge (x+4) at -4
        }
    }
}

void TankAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;  // Completely dead, don't draw
    
    int x = get_x();
    int y = get_y();
    
    // Bounds check - skip if completely off screen (allow partial for carousel wrap)
    if (x < -4 || x >= 68 || y < 4 || y > 70) return;

    if (get_health() == 0) {
        Alien::draw(matrix);
        return;
    }

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

    // Horseshoe crab tank - green/blue palette
    // Row +3: wide front edge
    sp(y + 3, x - 1, 25, 60, 70);
    sp(y + 3, x, 30, 70, 80);
    sp(y + 3, x + 1, 25, 60, 70);

    // Row +2: widest part of shell
    sp(y + 2, x - 3, 20, 50, 60);
    sp(y + 2, x - 2, 30, 70, 85);
    sp(y + 2, x - 1, 40, 90, 100);
    sp(y + 2, x, 45, 100, 110);
    sp(y + 2, x + 1, 40, 90, 100);
    sp(y + 2, x + 2, 30, 70, 85);
    sp(y + 2, x + 3, 20, 50, 60);

    // Row +1: upper shell
    sp(y + 1, x - 3, 25, 55, 65);
    sp(y + 1, x - 2, 35, 80, 90);
    sp(y + 1, x - 1, 50, 110, 80);
    sp(y + 1, x, 60, 130, 85);
    sp(y + 1, x + 1, 50, 110, 80);
    sp(y + 1, x + 2, 35, 80, 90);
    sp(y + 1, x + 3, 25, 55, 65);

    // Row 0: center ridge
    sp(y, x - 2, 30, 75, 85);
    sp(y, x - 1, 45, 105, 75);
    sp(y, x, 70, 150, 90);
    sp(y, x + 1, 45, 105, 75);
    sp(y, x + 2, 30, 75, 85);

    // Row -1: narrowing body
    sp(y - 1, x - 1, 30, 65, 80);
    sp(y - 1, x, 40, 85, 95);
    sp(y - 1, x + 1, 30, 65, 80);

    // Row -2: narrow tail base
    sp(y - 2, x, 25, 55, 75);

    // Row -3: tail spike
    sp(y - 3, x, 18, 40, 55);

    // Flashing twin engine dots flanking tail - dim magenta
    frameCounter++;
    if ((frameCounter / 2) % 2 == 0) {
        sp(y - 2, x - 1, 100, 25, 90);
        sp(y - 2, x + 1, 100, 25, 90);
    } else {
        sp(y - 2, x - 1, 35, 10, 30);
        sp(y - 2, x + 1, 35, 10, 30);
    }
}
