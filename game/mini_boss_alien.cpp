#include "mini_boss_alien.hpp"
#include <cstdlib>

MiniBossAlien::MiniBossAlien() : Alien(150, 1, 32, -8, MINI_BOSS) {
    shield = 50;
    shieldFlash = 0;
    shotCooldown = 30 + (rand() % 20);
    moveCounter = 0;
    horizontalDirection = (rand() % 2) * 2 - 1;
    frameCounter = 0;
    entryComplete = false;
    turretOffset = 0;
}

MiniBossAlien::MiniBossAlien(int x_in, int y_in) : Alien(150, 1, x_in, y_in, MINI_BOSS) {
    shield = 50;
    shieldFlash = 0;
    shotCooldown = 30 + (rand() % 20);
    moveCounter = 0;
    horizontalDirection = (rand() % 2) * 2 - 1;
    frameCounter = 0;
    entryComplete = false;
    turretOffset = 0;
}

MiniBossAlien::~MiniBossAlien() {
}

int MiniBossAlien::getShield() const { return shield; }
bool MiniBossAlien::isEntryComplete() const { return entryComplete; }
int MiniBossAlien::getTurretX() const { return turretOffset; }

void MiniBossAlien::updateTurret(int playerX) {
    int myX = get_x();
    int targetOffset = playerX - myX;
    // Clamp to -3..+3
    if (targetOffset > 3) targetOffset = 3;
    if (targetOffset < -3) targetOffset = -3;
    // Slowly track toward target
    if (turretOffset < targetOffset) turretOffset++;
    else if (turretOffset > targetOffset) turretOffset--;
}

void MiniBossAlien::move(int dx, int dy) {
    moveCounter++;
    if (shieldFlash > 0) shieldFlash--;

    // Entry - descend to y=14
    if (!entryComplete) {
        Alien::move(0, 1);
        if (get_y() >= 14) entryComplete = true;
        return;
    }

    // Horizontal patrol
    if (moveCounter % 20 == 0) {
        horizontalDirection *= -1;
    }
    int currentX = get_x();
    if (currentX <= 8) horizontalDirection = 1;
    if (currentX >= 55) horizontalDirection = -1;

    Alien::move(horizontalDirection, 0);
}

bool MiniBossAlien::shouldShoot() {
    if (!entryComplete) return false;
    shotCooldown--;
    return shotCooldown <= 0;
}

void MiniBossAlien::resetShotCooldown() {
    shotCooldown = 30 + (rand() % 15);
}

void MiniBossAlien::setShotCooldown(int cd) {
    shotCooldown = cd;
}

void MiniBossAlien::takeDamage(int damage) {
    if (!entryComplete) return;

    if (shield > 0) {
        if (shieldFlash <= 0) shieldFlash = 3;
        if (damage <= shield) {
            shield -= damage;
            return;
        } else {
            damage -= shield;
            shield = 0;
        }
    }
    Alien::takeDamage(damage);
}

void MiniBossAlien::erase(RGBMatrix *matrix) {
    int x = get_x();
    int y = get_y();
    // Clear a generous area to cover taller sprite + movement
    for (int dy = -8; dy <= 8; dy++) {
        for (int dx = -6; dx <= 6; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                matrix->SetPixel(py, px, 18, 10, 14);
            }
        }
    }
}

void MiniBossAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;

    int x = get_x();
    int y = get_y();

    if (x < 5 || x >= 59 || y < 5 || y > 68) return;

    if (get_health() == 0) {
        Alien::draw(matrix);
        return;
    }

    frameCounter++;
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

    // Green mini-boss colors
    int cr = 60, cg = 180, cb = 70;    // Core
    int br = 40, bg = 130, bb = 50;    // Body
    int ar = 30, ag = 90,  ab = 40;    // Armor
    int dr = 20, dg = 60,  db = 30;    // Dark edge

    // --- POINTY DAGGER SHAPE (5px wide, 13px tall) ---
    // Row -6: top spike tip
    sp(y - 6, x, dr, dg, db);

    // Row -5: spike
    sp(y - 5, x, ar, ag, ab);

    // Row -4: narrow head (3px)
    sp(y - 4, x - 1, dr, dg, db);
    sp(y - 4, x,     br, bg, bb);
    sp(y - 4, x + 1, dr, dg, db);

    // Row -3: widening (3px)
    sp(y - 3, x - 1, ar, ag, ab);
    sp(y - 3, x,     br, bg, bb);
    sp(y - 3, x + 1, ar, ag, ab);

    // Row -2: body (5px)
    for (int i = -2; i <= 2; i++) {
        if (i == 0) sp(y - 2, x + i, br, bg, bb);
        else sp(y - 2, x + i, ar, ag, ab);
    }

    // Row -1: widest with wing tips (7px)
    sp(y - 1, x - 3, dr, dg, db);
    sp(y - 1, x - 2, ar, ag, ab);
    sp(y - 1, x - 1, br, bg, bb);
    sp(y - 1, x,     br, bg, bb);
    sp(y - 1, x + 1, br, bg, bb);
    sp(y - 1, x + 2, ar, ag, ab);
    sp(y - 1, x + 3, dr, dg, db);

    // Row 0: core eye row (5px)
    sp(y, x - 2, ar, ag, ab);
    sp(y, x - 1, br, bg, bb);
    sp(y, x,     cr, cg, cb);  // Core eye
    sp(y, x + 1, br, bg, bb);
    sp(y, x + 2, ar, ag, ab);

    // Row +1: narrowing (5px)
    for (int i = -2; i <= 2; i++) {
        if (i == 0) sp(y + 1, x + i, br, bg, bb);
        else sp(y + 1, x + i, ar, ag, ab);
    }

    // Row +2: narrow (3px)
    sp(y + 2, x - 1, ar, ag, ab);
    sp(y + 2, x,     br, bg, bb);
    sp(y + 2, x + 1, ar, ag, ab);

    // Row +3: lower body (3px)
    sp(y + 3, x - 1, dr, dg, db);
    sp(y + 3, x,     ar, ag, ab);
    sp(y + 3, x + 1, dr, dg, db);

    // Row +4: barrel
    sp(y + 4, x, br, bg, bb);

    // Row +5: gun tip
    sp(y + 5, x, cr, cg, cb);

    // Side spike protrusions at widest point
    sp(y - 1, x - 4, cr, cg, cb);  // Left spike tip
    sp(y - 1, x + 4, cr, cg, cb);  // Right spike tip

    // Ornament gems
    sp(y - 2, x - 1, cr, cg, cb);
    sp(y - 2, x + 1, cr, cg, cb);

    // --- FLICKERING PINK ENGINES (back/top) ---
    if ((frameCounter / 2) % 2 == 0) {
        sp(y - 5, x - 1, 120, 30, 100);
        sp(y - 5, x + 1, 120, 30, 100);
    } else {
        sp(y - 5, x - 1, 45, 12, 38);
        sp(y - 5, x + 1, 45, 12, 38);
    }

    // Shield flash — conforms to pointy front shape
    if (shieldFlash > 0 && shield > 0) {
        int sb = 80 + shieldFlash * 50;
        auto edge = [matrix, sb](int py, int px) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 30, 60, sb);
        };
        edge(y + 2, x - 2); edge(y + 2, x + 2);
        edge(y + 3, x - 1); edge(y + 3, x + 1);
        edge(y + 4, x - 1); edge(y + 4, x + 1);
        edge(y + 5, x);
        edge(y + 6, x);
    }
}
