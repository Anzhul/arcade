#include "launcher_alien.hpp"
#include <cstdlib>

LauncherAlien::LauncherAlien() : Alien(45, 1, 32, -8, LAUNCHER) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    moveCounter = 0;
    shotCooldown = 15 + (rand() % 10);
    anchorY = 8 + (rand() % 12);  // Anchor between y=8 and y=20
    anchored = false;
}

LauncherAlien::LauncherAlien(int x_in, int y_in) : Alien(45, 1, x_in, y_in, LAUNCHER) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    moveCounter = 0;
    shotCooldown = 15 + (rand() % 10);
    anchorY = 8 + (rand() % 12);
    anchored = false;
}

LauncherAlien::~LauncherAlien() {
}

void LauncherAlien::move(int dx, int dy) {
    moveCounter++;
    shotCooldown--;

    if (!anchored) {
        // Descend until reaching anchor point
        if (get_y() < anchorY) {
            Alien::move(0, dy);
        } else {
            anchored = true;
        }
    }

    if (anchored) {
        // Slow horizontal drift only
        if (moveCounter % 25 == 0) {
            if (rand() % 3 == 0) {
                horizontalDirection *= -1;
            }
        }

        int currentX = get_x();
        if (currentX <= 5 || currentX >= 58) {
            horizontalDirection *= -1;
        }

        // Move horizontally only — no vertical movement
        Alien::move(horizontalDirection, 0);
    }
}

bool LauncherAlien::shouldShoot() {
    if (!anchored) return false;
    if (shotCooldown <= 0) {
        return true;
    }
    return false;
}

void LauncherAlien::resetShotCooldown() {
    shotCooldown = 15 + (rand() % 10);  // Faster fire rate: 40-65 frames
}

void LauncherAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;

    int x = get_x();
    int y = get_y();

    if (x < 3 || x >= 61 || y < 3 || y > 66) return;

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

    // Launcher alien - pink with green accents
    // Row -2: antenna tips - green
    sp(y - 2, x - 1, 50, 140, 60);
    sp(y - 2, x + 1, 50, 140, 60);

    // Row -1: upper hull - pink with green center
    sp(y - 1, x - 1, 180, 50, 120);
    sp(y - 1, x, 70, 160, 70);      // Green center accent
    sp(y - 1, x + 1, 180, 50, 120);

    // Row 0: main body - wide, pink with green edges
    sp(y, x - 2, 45, 120, 50);      // Green edge
    sp(y, x - 1, 200, 60, 140);
    sp(y, x, 240, 80, 160);         // Bright pink center
    sp(y, x + 1, 200, 60, 140);
    sp(y, x + 2, 45, 120, 50);      // Green edge

    // Row +1: launcher tube
    sp(y + 1, x - 2, 40, 100, 45);  // Green
    sp(y + 1, x - 1, 160, 45, 110);
    sp(y + 1, x, 200, 60, 140);
    sp(y + 1, x + 1, 160, 45, 110);
    sp(y + 1, x + 2, 40, 100, 45);  // Green

    // Row +2: missile bay (blinks when about to fire)
    if (shotCooldown < 20 && (moveCounter / 3) % 2 == 0) {
        // Blinking warning - bright pink
        sp(y + 2, x - 1, 255, 60, 180);
        sp(y + 2, x, 255, 90, 200);
        sp(y + 2, x + 1, 255, 60, 180);
    } else {
        sp(y + 2, x - 1, 100, 30, 70);
        sp(y + 2, x, 120, 40, 85);
        sp(y + 2, x + 1, 100, 30, 70);
    }
}
