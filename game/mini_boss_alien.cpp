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
    shotCooldown = 30 + (rand() % 15);  // Faster: every 30-45 frames
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
    // Clear a generous area to cover movement + turret range
    for (int dy = -6; dy <= 7; dy++) {
        for (int dx = -7; dx <= 7; dx++) {
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

    // Row -4: crown spike
    sp(y - 4, x, ar, ag, ab);

    // Row -3: head
    sp(y - 3, x - 1, dr, dg, db);
    sp(y - 3, x,     br, bg, bb);
    sp(y - 3, x + 1, dr, dg, db);

    // Row -2: widening (7px)
    for (int i = -3; i <= 3; i++) {
        if (i >= -1 && i <= 1) sp(y - 2, x + i, br, bg, bb);
        else sp(y - 2, x + i, ar, ag, ab);
    }

    // Row -1: widest (9px)
    for (int i = -4; i <= 4; i++) {
        if (i >= -1 && i <= 1) sp(y - 1, x + i, br, bg, bb);
        else if (i >= -3 && i <= 3) sp(y - 1, x + i, ar, ag, ab);
        else sp(y - 1, x + i, dr, dg, db);
    }

    // Row 0: center with core eye (9px)
    for (int i = -4; i <= 4; i++) {
        if (i == 0) sp(y, x, cr, cg, cb);
        else if (i >= -2 && i <= 2) sp(y, x + i, br, bg, bb);
        else if (i >= -3 && i <= 3) sp(y, x + i, ar, ag, ab);
        else sp(y, x + i, dr, dg, db);
    }

    // Row +1: lower body (7px)
    for (int i = -3; i <= 3; i++) {
        if (i >= -1 && i <= 1) sp(y + 1, x + i, br, bg, bb);
        else sp(y + 1, x + i, ar, ag, ab);
    }

    // Row +2: narrowing (5px)
    for (int i = -2; i <= 2; i++) {
        if (i == 0) sp(y + 2, x + i, br, bg, bb);
        else sp(y + 2, x + i, ar, ag, ab);
    }

    // Turret is hidden — missiles fire from turret offset position

    // --- TRIANGULAR SIDE PROTRUSIONS ---
    sp(y - 1, x - 5, cr, cg, cb);
    sp(y,     x - 5, dr, dg, db);
    sp(y - 1, x + 5, cr, cg, cb);
    sp(y,     x + 5, dr, dg, db);

    // Ornament gems
    sp(y - 2, x - 2, cr, cg, cb);
    sp(y - 2, x + 2, cr, cg, cb);

    // --- FLICKERING PINK ENGINES (back/top) ---
    if ((frameCounter / 2) % 2 == 0) {
        sp(y - 4, x - 2, 120, 30, 100);
        sp(y - 4, x + 2, 120, 30, 100);
    } else {
        sp(y - 4, x - 2, 45, 12, 38);
        sp(y - 4, x + 2, 45, 12, 38);
    }

    // Shield flash — conforms to front shape
    if (shieldFlash > 0 && shield > 0) {
        int sb = 80 + shieldFlash * 50;
        auto edge = [matrix, sb](int py, int px) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 30, 60, sb);
        };
        // Follows bottom contour: widest at row +1, narrowing to gun
        edge(y + 2, x - 4); edge(y + 2, x + 4);  // Side edges
        for (int i = -3; i <= 3; i++) edge(y + 2, x + i);  // Row +2 base
        edge(y + 3, x - 2); edge(y + 3, x - 1);
        edge(y + 3, x + 1); edge(y + 3, x + 2);  // Narrowing
        edge(y + 4, x);  // Gun tip
    }
}
