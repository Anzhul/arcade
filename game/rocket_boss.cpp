#include "rocket_boss.hpp"
#include <cstdlib>

RocketBoss::RocketBoss() : Alien(200, 1, 32, -8, ROCKET_BOSS) {
    shield = 60;
    shieldFlash = 0;
    shotCooldown = 50;
    moveCounter = 0;
    horizontalDirection = (rand() % 2) * 2 - 1;
    frameCounter = 0;
    entryComplete = false;
    attackMode = 0;
    rocketBurst = 0;
}

RocketBoss::RocketBoss(int x_in, int y_in) : Alien(200, 1, x_in, y_in, ROCKET_BOSS) {
    shield = 60;
    shieldFlash = 0;
    shotCooldown = 50;
    moveCounter = 0;
    horizontalDirection = (rand() % 2) * 2 - 1;
    frameCounter = 0;
    entryComplete = false;
    attackMode = 0;
    rocketBurst = 0;
}

RocketBoss::~RocketBoss() {
}

int RocketBoss::getShield() const { return shield; }
bool RocketBoss::isEntryComplete() const { return entryComplete; }

void RocketBoss::move(int dx, int dy) {
    moveCounter++;
    frameCounter++;
    if (shieldFlash > 0) shieldFlash--;

    // Entry - descend to y=12
    if (!entryComplete) {
        Alien::move(0, 1);
        if (get_y() >= 12) entryComplete = true;
        return;
    }

    // Horizontal patrol
    int changeRate = 16;
    if (moveCounter % changeRate == 0) {
        if (rand() % 2 == 0) horizontalDirection *= -1;
    }
    int currentX = get_x();
    if (currentX <= 8) horizontalDirection = 1;
    if (currentX >= 55) horizontalDirection = -1;

    Alien::move(horizontalDirection, 0);
}

int RocketBoss::getPhase() {
    // Phase 2 at half health
    if (get_health() <= 100) return 2;
    return 1;
}

bool RocketBoss::shouldShoot() {
    if (!entryComplete) return false;
    shotCooldown--;
    if (shotCooldown > 0) return false;

    if (rocketBurst > 0) {
        // In the middle of a rocket burst
        attackMode = 0;
        return true;
    }

    // Decide next attack: random scatter or rocket burst
    if (rand() % 3 == 0) {
        // Scatter shot
        attackMode = 1;
        return true;
    } else {
        // Start rocket burst
        int phase = getPhase();
        rocketBurst = (phase == 2) ? 6 : 3;  // Phase 2: 6 rockets (3 per side)
        attackMode = 0;
        return true;
    }
}

void RocketBoss::resetShotCooldown() {
    if (attackMode == 0) {
        rocketBurst--;
        if (rocketBurst > 0) {
            shotCooldown = 6;   // Fast between burst rockets
        } else {
            shotCooldown = 25 + (rand() % 15);  // Pause after burst
        }
    } else {
        shotCooldown = 15 + (rand() % 10);  // Pause after scatter
    }
}

void RocketBoss::takeDamage(int damage) {
    if (!entryComplete) return;

    if (shield > 0) {
        if (shieldFlash <= 0) shieldFlash = 4;
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

void RocketBoss::erase(RGBMatrix *matrix) {
    int x = get_x();
    int y = get_y();
    for (int dy = -8; dy <= 8; dy++) {
        for (int dx = -6; dx <= 6; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 18, 10, 14);
        }
    }
}

void RocketBoss::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;

    int x = get_x();
    int y = get_y();

    if (x < 5 || x >= 59 || y < 5 || y > 68) return;

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

    // Dark red/crimson rocket boss palette
    int cr = 180, cg = 50, cb = 40;    // Core - bright crimson
    int br = 130, bg = 35, bb = 30;    // Body
    int ar = 90,  ag = 25, ab = 25;    // Armor
    int dr = 55,  dg = 18, db = 18;    // Dark edge

    // Row -6: top spike
    sp(y - 6, x, dr, dg, db);

    // Row -5: spike widening
    sp(y - 5, x - 1, dr, dg, db);
    sp(y - 5, x, ar, ag, ab);
    sp(y - 5, x + 1, dr, dg, db);

    // Row -4: head (3px)
    sp(y - 4, x - 1, ar, ag, ab);
    sp(y - 4, x, br, bg, bb);
    sp(y - 4, x + 1, ar, ag, ab);

    // Row -3: widening (5px)
    sp(y - 3, x - 2, dr, dg, db);
    sp(y - 3, x - 1, br, bg, bb);
    sp(y - 3, x, br, bg, bb);
    sp(y - 3, x + 1, br, bg, bb);
    sp(y - 3, x + 2, dr, dg, db);

    // Row -2: body (5px) with orange accents
    sp(y - 2, x - 2, ar, ag, ab);
    sp(y - 2, x - 1, 160, 80, 30);  // Orange accent
    sp(y - 2, x, br, bg, bb);
    sp(y - 2, x + 1, 160, 80, 30);  // Orange accent
    sp(y - 2, x + 2, ar, ag, ab);

    // Row -1: widest with wing prongs (9px)
    sp(y - 1, x - 4, cr, cg, cb);   // Wing tip
    sp(y - 1, x - 3, ar, ag, ab);
    sp(y - 1, x - 2, br, bg, bb);
    sp(y - 1, x - 1, br, bg, bb);
    sp(y - 1, x, cr, cg, cb);       // Center
    sp(y - 1, x + 1, br, bg, bb);
    sp(y - 1, x + 2, br, bg, bb);
    sp(y - 1, x + 3, ar, ag, ab);
    sp(y - 1, x + 4, cr, cg, cb);   // Wing tip

    // Row 0: core row (7px)
    sp(y, x - 3, ar, ag, ab);
    sp(y, x - 2, br, bg, bb);
    sp(y, x - 1, cr, cg, cb);
    sp(y, x, 220, 70, 50);          // Bright core eye
    sp(y, x + 1, cr, cg, cb);
    sp(y, x + 2, br, bg, bb);
    sp(y, x + 3, ar, ag, ab);

    // Row +1: lower body (7px) with rocket pods
    sp(y + 1, x - 3, 140, 60, 20);  // Left rocket pod - orange
    sp(y + 1, x - 2, br, bg, bb);
    sp(y + 1, x - 1, br, bg, bb);
    sp(y + 1, x, br, bg, bb);
    sp(y + 1, x + 1, br, bg, bb);
    sp(y + 1, x + 2, br, bg, bb);
    sp(y + 1, x + 3, 140, 60, 20);  // Right rocket pod - orange

    // Row +2: narrowing (5px) with rocket tubes
    sp(y + 2, x - 3, 160, 70, 25);  // Left tube
    sp(y + 2, x - 2, ar, ag, ab);
    sp(y + 2, x, br, bg, bb);
    sp(y + 2, x + 2, ar, ag, ab);
    sp(y + 2, x + 3, 160, 70, 25);  // Right tube

    // Row +3: narrow front (3px) + rocket tips
    sp(y + 3, x - 3, 180, 80, 30);  // Left rocket tip
    sp(y + 3, x - 1, ar, ag, ab);
    sp(y + 3, x, br, bg, bb);
    sp(y + 3, x + 1, ar, ag, ab);
    sp(y + 3, x + 3, 180, 80, 30);  // Right rocket tip

    // Row +4: center barrel + scatter ports
    sp(y + 4, x - 1, dr, dg, db);
    sp(y + 4, x, ar, ag, ab);
    sp(y + 4, x + 1, dr, dg, db);

    // Row +5: gun tip
    sp(y + 5, x, cr, cg, cb);

    // Rocket pod glow — blinks when in rocket mode
    if (attackMode == 0 && (frameCounter / 3) % 2 == 0) {
        sp(y + 3, x - 3, 255, 120, 40);
        sp(y + 3, x + 3, 255, 120, 40);
    }

    // Scatter port glow — blinks when in scatter mode
    if (attackMode == 1 && (frameCounter / 3) % 2 == 0) {
        sp(y + 4, x - 1, 255, 80, 80);
        sp(y + 4, x + 1, 255, 80, 80);
        sp(y + 5, x, 255, 100, 60);
    }

    // Engines (back/top) — flickering magenta
    if ((frameCounter / 2) % 2 == 0) {
        sp(y - 5, x - 2, 130, 30, 110);
        sp(y - 5, x + 2, 130, 30, 110);
    } else {
        sp(y - 5, x - 2, 50, 12, 42);
        sp(y - 5, x + 2, 50, 12, 42);
    }

    // Shield flash — outline
    if (shieldFlash > 0 && shield > 0) {
        int sb = 80 + shieldFlash * 45;
        auto edge = [matrix, sb](int py, int px) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 30, 60, sb);
        };
        edge(y + 3, x - 2); edge(y + 3, x + 2);
        edge(y + 4, x - 2); edge(y + 4, x + 2);
        edge(y + 5, x - 1); edge(y + 5, x + 1);
        edge(y + 6, x);
        edge(y + 1, x - 4); edge(y + 1, x + 4);
        edge(y, x - 4); edge(y, x + 4);
        edge(y - 1, x - 5); edge(y - 1, x + 5);
        edge(y - 2, x - 3); edge(y - 2, x + 3);
    }
}
