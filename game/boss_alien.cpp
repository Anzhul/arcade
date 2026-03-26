#include "boss_alien.hpp"
#include <cstdlib>

BossAlien::BossAlien() : Alien(600, 1, 32, -12, BOSS) {
    phase = 1;
    maxHealth = 600;
    shield = 200;
    shieldFlash = 0;
    shotCooldown = 60;
    moveCounter = 0;
    horizontalDirection = 1;
    phaseTransitionTimer = 0;
    minionSpawnCooldown = 150;
    frameCounter = 0;
    entryComplete = false;
    shieldBreakFlash = 0;
    redLightIntensity = 0;
    explosionRequest = false;
}

BossAlien::BossAlien(int x_in, int y_in) : Alien(600, 1, x_in, y_in, BOSS) {
    phase = 1;
    maxHealth = 600;
    shield = 200;
    shieldFlash = 0;
    shotCooldown = 60;
    moveCounter = 0;
    horizontalDirection = 1;
    phaseTransitionTimer = 0;
    minionSpawnCooldown = 150;
    frameCounter = 0;
    entryComplete = false;
    shieldBreakFlash = 0;
    redLightIntensity = 0;
    explosionRequest = false;
}

BossAlien::~BossAlien() {
}

int BossAlien::getPhase() const { return phase; }
int BossAlien::getShield() const { return shield; }
bool BossAlien::isEntryComplete() const { return entryComplete; }

bool BossAlien::wantsExplosion() const { return explosionRequest; }
int BossAlien::getRedIntensity() const { return redLightIntensity; }

void BossAlien::move(int dx, int dy) {
    moveCounter++;
    if (shieldFlash > 0) shieldFlash--;
    if (shieldBreakFlash > 0) shieldBreakFlash--;
    explosionRequest = false;  // Reset each frame
    if (phaseTransitionTimer > 0) {
        phaseTransitionTimer--;
        // Still invulnerable but keep moving

        // Phase 1->2 transition: gradually add red lights
        if (phase == 2 && redLightIntensity < 120) {
            redLightIntensity += 4;
        }

        // Phase 2->3 transition: explosions on the boss body
        if (phase == 3 && phaseTransitionTimer % 5 == 0) {
            explosionRequest = true;
        }
        if (phase == 3 && redLightIntensity < 200) {
            redLightIntensity += 6;
        }
    }

    // Entry animation - scroll down to y=12
    if (!entryComplete) {
        Alien::move(0, 1);
        if (get_y() >= 12) {
            entryComplete = true;
        }
        return;
    }

    // Phase-dependent horizontal movement
    int currentX = get_x();

    if (phase == 1) {
        // Slow drift
        if (moveCounter % 25 == 0) {
            horizontalDirection *= -1;
        }
    } else if (phase == 2) {
        // Zigzag
        if (moveCounter % 15 == 0) {
            horizontalDirection *= -1;
        }
    } else {
        // Erratic
        if (moveCounter % 8 == 0) {
            if (rand() % 2 == 0) {
                horizontalDirection *= -1;
            }
        }
    }

    // Bounce off edges (boss is wide, ~13px each side)
    if (currentX <= 15) horizontalDirection = 1;
    if (currentX >= 48) horizontalDirection = -1;

    Alien::move(horizontalDirection, 0);  // No vertical movement after entry
}

bool BossAlien::shouldShoot() {
    if (!entryComplete || phaseTransitionTimer > 0) return false;
    shotCooldown--;
    return shotCooldown <= 0;
}

void BossAlien::resetShotCooldown() {
    if (phase == 1) {
        shotCooldown = 40 + (rand() % 20);
    } else if (phase == 2) {
        shotCooldown = 25 + (rand() % 15);
    } else {
        shotCooldown = 10 + (rand() % 8);
    }
}

bool BossAlien::shouldSpawnMinion() {
    if (!entryComplete || phase == 1 || phaseTransitionTimer > 0) return false;
    minionSpawnCooldown--;
    return minionSpawnCooldown <= 0;
}

void BossAlien::resetMinionCooldown() {
    if (phase == 2) {
        minionSpawnCooldown = 100 + (rand() % 60);
    } else {
        minionSpawnCooldown = 60 + (rand() % 40);
    }
}

void BossAlien::takeDamage(int damage) {
    if (!entryComplete || phaseTransitionTimer > 0) return;  // Invulnerable

    // Shield absorbs damage in phase 1
    if (shield > 0) {
        shieldFlash = 5;
        if (damage <= shield) {
            shield -= damage;
            return;
        } else {
            damage -= shield;
            shield = 0;
            shieldBreakFlash = 10;  // Bright blue flash when shield shatters
        }
    }

    // Apply remaining damage to health via base class
    Alien::takeDamage(damage);

    // Check phase transitions
    int hp = get_health();
    if (hp > 0 && phase == 1 && shield <= 0) {
        phase = 2;
        phaseTransitionTimer = 30;
    } else if (hp > 0 && hp <= 200 && phase == 2) {
        phase = 3;
        phaseTransitionTimer = 30;
    }
}

void BossAlien::erase(RGBMatrix *matrix) {
    int x = get_x();
    int y = get_y();
    for (int dy = -10; dy <= 11; dy++) {
        for (int dx = -15; dx <= 15; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                matrix->SetPixel(py, px, 18, 10, 14);
            }
        }
    }
}

void BossAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;

    int x = get_x();
    int y = get_y();

    if (x < 0 || x >= 64 || y < -10 || y > 70) return;

    if (get_health() == 0) {
        Alien::draw(matrix);
        return;
    }

    frameCounter++;

    // Phase transition — brief invulnerability, but still draw normally
    // (the hit flash + phase color shift handles the visual feedback)

    // Hit flash
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

    // Phase 3 damage flicker - randomly skip pixels
    bool skipPixel = false;

    // Color palette - dusty plum/violet (matches elite aliens)
    // Phases 1 & 2 share the same appearance
    int cr, cg, cb;       // Core color
    int br, bg, bb;       // Body color
    int ar, ag, ab;       // Armor/edge color
    int dr, dg, db;       // Dark edge color

    if (phase <= 2) {
        cr = 160; cg = 90;  cb = 170;   // Bright plum core
        br = 120; bg = 60;  bb = 130;   // Dusty plum body
        ar = 85;  ag = 40;  ab = 95;    // Dark violet armor
        dr = 55;  dg = 28;  db = 65;    // Darkest edge
    } else {
        // Phase 3 - pulsing violet
        int pulse = ((frameCounter / 3) % 2 == 0) ? 35 : 0;
        cr = 180 + pulse; cg = 70; cb = 190 + pulse;
        br = 140 + pulse; bg = 45; bb = 150;
        ar = 100; ag = 30; ab = 110;
        dr = 65;  dg = 20; db = 75;
    }

    // --- MASSIVE BOSS SPRITE (27px wide, 19px tall) ---

    // Row -9: crown spikes
    sp(y - 9, x - 2, dr, dg, db);
    sp(y - 9, x,     ar, ag, ab);
    sp(y - 9, x + 2, dr, dg, db);

    // Row -8: horn extensions
    sp(y - 8, x - 4, dr, dg, db);
    sp(y - 8, x - 2, ar, ag, ab);
    for (int i = -1; i <= 1; i++) sp(y - 8, x + i, br, bg, bb);
    sp(y - 8, x + 2, ar, ag, ab);
    sp(y - 8, x + 4, dr, dg, db);

    // Row -7: head widening (15px)
    for (int i = -7; i <= 7; i++) {
        if (phase == 3 && (rand() % 8 == 0)) continue;
        if (i >= -2 && i <= 2) sp(y - 7, x + i, br, bg, bb);
        else if (i >= -4 && i <= 4) sp(y - 7, x + i, ar, ag, ab);
        else sp(y - 7, x + i, dr, dg, db);
    }

    // Row -6: shoulder ridge (19px)
    for (int i = -9; i <= 9; i++) {
        if (phase == 3 && (rand() % 8 == 0)) continue;
        if (i >= -2 && i <= 2) sp(y - 6, x + i, br, bg, bb);
        else if (i >= -5 && i <= 5) sp(y - 6, x + i, ar, ag, ab);
        else sp(y - 6, x + i, dr, dg, db);
    }

    // Row -5: widening (23px)
    for (int i = -11; i <= 11; i++) {
        if (phase == 3 && (rand() % 10 == 0)) continue;
        if (i >= -2 && i <= 2) sp(y - 5, x + i, br, bg, bb);
        else if (i >= -6 && i <= 6) sp(y - 5, x + i, ar, ag, ab);
        else sp(y - 5, x + i, dr, dg, db);
    }

    // Row -4: wide body (25px)
    for (int i = -12; i <= 12; i++) {
        if (phase == 3 && (rand() % 10 == 0)) continue;
        if (i >= -3 && i <= 3) sp(y - 4, x + i, br, bg, bb);
        else if (i >= -7 && i <= 7) sp(y - 4, x + i, ar, ag, ab);
        else sp(y - 4, x + i, dr, dg, db);
    }

    // Rows -3 to +1: widest section (27px) with CORE EYE at row -1
    for (int row = -3; row <= 1; row++) {
        for (int i = -13; i <= 13; i++) {
            if (phase == 3 && (rand() % 10 == 0)) continue;
            if (row == -1 && i == 0) sp(y + row, x, cr, cg, cb);  // CORE EYE
            else if (i >= -3 && i <= 3) sp(y + row, x + i, br, bg, bb);
            else if (i >= -7 && i <= 7) sp(y + row, x + i, ar, ag, ab);
            else sp(y + row, x + i, dr, dg, db);
        }
    }

    // Row +2: narrowing (25px)
    for (int i = -12; i <= 12; i++) {
        if (phase == 3 && (rand() % 10 == 0)) continue;
        if (i >= -3 && i <= 3) sp(y + 2, x + i, br, bg, bb);
        else if (i >= -7 && i <= 7) sp(y + 2, x + i, ar, ag, ab);
        else sp(y + 2, x + i, dr, dg, db);
    }

    // Row +3: narrowing (21px)
    for (int i = -10; i <= 10; i++) {
        if (i >= -2 && i <= 2) sp(y + 3, x + i, br, bg, bb);
        else if (i >= -6 && i <= 6) sp(y + 3, x + i, ar, ag, ab);
        else sp(y + 3, x + i, dr, dg, db);
    }

    // Row +4: narrowing (17px)
    for (int i = -8; i <= 8; i++) {
        if (i >= -2 && i <= 2) sp(y + 4, x + i, br, bg, bb);
        else if (i >= -5 && i <= 5) sp(y + 4, x + i, ar, ag, ab);
        else sp(y + 4, x + i, dr, dg, db);
    }

    // Row +5: lower hull (13px)
    for (int i = -6; i <= 6; i++) {
        if (i >= -1 && i <= 1) sp(y + 5, x + i, br, bg, bb);
        else sp(y + 5, x + i, ar, ag, ab);
    }

    // Row +6: exhaust (9px)
    for (int i = -4; i <= 4; i++) {
        if (i == 0) sp(y + 6, x + i, br, bg, bb);
        else sp(y + 6, x + i, ar, ag, ab);
    }

    // Row +7: gun barrels
    sp(y + 7, x - 2, ar, ag, ab);
    sp(y + 7, x,     cr, cg, cb);
    sp(y + 7, x + 2, ar, ag, ab);

    // Row +8: gun tip
    sp(y + 8, x, cr, cg, cb);

    // --- OBELISK TOWERS (tall spires on top) ---
    for (int i = 0; i < 5; i++) {
        int shade = (i == 0 || i == 4) ? 0 : (i == 2 ? 2 : 1);
        int colr = shade == 0 ? dr : (shade == 1 ? ar : br);
        int colg = shade == 0 ? dg : (shade == 1 ? ag : bg);
        int colb = shade == 0 ? db : (shade == 1 ? ab : bb);
        sp(y - 8 - i, x - 9, colr, colg, colb);
        sp(y - 8 - i, x + 9, colr, colg, colb);
    }

    // --- THICK TRIANGULAR PINCERS ---
    // Left pincer: thick triangle pointing forward (down)
    // Base at row +1, tip at row +7
    sp(y + 1, x - 11, dr, dg, db);  sp(y + 1, x - 10, ar, ag, ab);  sp(y + 1, x - 9, ar, ag, ab);
    sp(y + 2, x - 11, ar, ag, ab);  sp(y + 2, x - 10, br, bg, bb);  sp(y + 2, x - 9, ar, ag, ab);
    sp(y + 3, x - 10, ar, ag, ab);  sp(y + 3, x - 9, br, bg, bb);
    sp(y + 4, x - 10, br, bg, bb);  sp(y + 4, x - 9, ar, ag, ab);
    sp(y + 5, x - 9, ar, ag, ab);   sp(y + 5, x - 8, ar, ag, ab);
    sp(y + 6, x - 8, br, bg, bb);
    sp(y + 7, x - 7, cr, cg, cb);   // Pincer tip (bright)

    // Right pincer (mirrored)
    sp(y + 1, x + 11, dr, dg, db);  sp(y + 1, x + 10, ar, ag, ab);  sp(y + 1, x + 9, ar, ag, ab);
    sp(y + 2, x + 11, ar, ag, ab);  sp(y + 2, x + 10, br, bg, bb);  sp(y + 2, x + 9, ar, ag, ab);
    sp(y + 3, x + 10, ar, ag, ab);  sp(y + 3, x + 9, br, bg, bb);
    sp(y + 4, x + 10, br, bg, bb);  sp(y + 4, x + 9, ar, ag, ab);
    sp(y + 5, x + 9, ar, ag, ab);   sp(y + 5, x + 8, ar, ag, ab);
    sp(y + 6, x + 8, br, bg, bb);
    sp(y + 7, x + 7, cr, cg, cb);   // Pincer tip (bright)

    // --- ORNAMENT GEMS ---
    sp(y - 6, x - 6, cr, cg, cb);
    sp(y - 6, x + 6, cr, cg, cb);
    sp(y - 1, x - 5, cr, cg, cb);
    sp(y - 1, x + 5, cr, cg, cb);
    sp(y + 3, x - 4, cr, cg, cb);
    sp(y + 3, x + 4, cr, cg, cb);

    // --- RED WARNING LIGHTS (grow after phase 1) ---
    if (redLightIntensity > 0) {
        int ri = redLightIntensity;
        int pulse = ((frameCounter / 4) % 2 == 0) ? ri : ri / 3;
        // Outer wing warning lights
        sp(y - 1, x - 12, pulse, 10, 10);
        sp(y - 1, x + 12, pulse, 10, 10);
        sp(y, x - 12, pulse, 10, 10);
        sp(y, x + 12, pulse, 10, 10);
        if (ri > 60) {
            sp(y - 4, x - 8, pulse, 8, 8);
            sp(y - 4, x + 8, pulse, 8, 8);
            sp(y + 2, x - 10, pulse, 8, 8);
            sp(y + 2, x + 10, pulse, 8, 8);
        }
        if (ri > 120) {
            sp(y - 6, x - 5, pulse, 5, 5);
            sp(y - 6, x + 5, pulse, 5, 5);
            sp(y + 1, x - 8, pulse, 5, 5);
            sp(y + 1, x + 8, pulse, 5, 5);
        }
    }

    // --- WEAK POINTS (after shield depleted) ---
    // Three glowing targets that cycle red -> yellow -> orange
    // Left: (x-7, y+3), Center: (x, y+7), Right: (x+7, y+3)
    if (shield <= 0) {
        int wpPhase = (frameCounter / 8) % 3;  // Cycle every 8 frames
        int wpPulse = ((frameCounter / 2) % 2 == 0) ? 30 : 0;

        // Color cycle: 0=red, 1=yellow, 2=orange
        int wr, wg, wb;  // Weak point center color
        int er, eg, eb;  // Weak point edge color
        if (wpPhase == 0) {        // Red
            wr = 220 + wpPulse; wg = 40; wb = 25;
            er = 160 + wpPulse; eg = 25; eb = 15;
        } else if (wpPhase == 1) { // Yellow
            wr = 230 + wpPulse; wg = 200 + wpPulse; wb = 40;
            er = 180 + wpPulse; eg = 150; eb = 25;
        } else {                    // Orange
            wr = 230 + wpPulse; wg = 110; wb = 25;
            er = 180 + wpPulse; eg = 75; eb = 15;
        }
        if (wr > 255) wr = 255;
        if (wg > 255) wg = 255;
        if (er > 255) er = 255;
        if (eg > 255) eg = 255;

        // Draw all three weak points with the same cycling color
        // Left weak point
        sp(y + 2, x - 7, er, eg, eb);
        sp(y + 3, x - 8, er, eg, eb);
        sp(y + 3, x - 7, wr, wg, wb);
        sp(y + 3, x - 6, er, eg, eb);
        sp(y + 4, x - 7, er, eg, eb);

        // Center weak point
        sp(y + 6, x,     er, eg, eb);
        sp(y + 7, x - 1, er, eg, eb);
        sp(y + 7, x,     wr, wg, wb);
        sp(y + 7, x + 1, er, eg, eb);
        sp(y + 8, x,     er, eg, eb);

        // Right weak point
        sp(y + 2, x + 7, er, eg, eb);
        sp(y + 3, x + 8, er, eg, eb);
        sp(y + 3, x + 7, wr, wg, wb);
        sp(y + 3, x + 6, er, eg, eb);
        sp(y + 4, x + 7, er, eg, eb);

        // Red weapon streaks
        sp(y + 5, x - 4, 160, 30, 35);
        sp(y + 4, x - 5, 140, 25, 30);
        sp(y + 5, x + 4, 160, 30, 35);
        sp(y + 4, x + 5, 140, 25, 30);
    }

    // Shield only visible when hit or breaking
    if (shieldFlash > 0 || shieldBreakFlash > 0) {
        int sr, sg, sb;
        if (shieldBreakFlash > 0) {
            int intensity = shieldBreakFlash * 25;
            sr = 100 + intensity;
            sg = 120 + intensity;
            sb = 200 + (intensity / 2);
            if (sr > 255) sr = 255;
            if (sg > 255) sg = 255;
            if (sb > 255) sb = 255;
        } else {
            sr = 50; sg = 80; sb = 80 + shieldFlash * 35;
        }
        auto edge = [matrix, sr, sg, sb](int py, int px) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, sr, sg, sb);
        };
        // Shield traces front half outline of larger sprite
        edge(y, x - 14); edge(y, x + 14);
        edge(y + 1, x - 14); edge(y + 1, x + 14);
        edge(y + 2, x - 13); edge(y + 2, x + 13);
        edge(y + 3, x - 11); edge(y + 3, x + 11);
        edge(y + 4, x - 9); edge(y + 4, x + 9);
        edge(y + 5, x - 7); edge(y + 5, x + 7);
        edge(y + 6, x - 5); edge(y + 6, x + 5);
        edge(y + 7, x - 3); edge(y + 7, x + 3);
        edge(y + 8, x - 1); edge(y + 8, x + 1);
        edge(y + 9, x);
    }
}
