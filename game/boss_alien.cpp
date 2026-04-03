#include "boss_alien.hpp"
#include <cstdlib>

BossAlien::BossAlien() : Alien(800, 1, 32, -12, BOSS) {
    phase = 1;
    maxHealth = 800;
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
    bossDeathTimer = 0;
    bossDying = false;
    attackPattern = 0;
    beamTimer = 0;
    beamChargeTimer = 0;
}

BossAlien::BossAlien(int x_in, int y_in) : Alien(800, 1, x_in, y_in, BOSS) {
    phase = 1;
    maxHealth = 800;
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
    bossDeathTimer = 0;
    bossDying = false;
    attackPattern = 0;
    beamTimer = 0;
    beamChargeTimer = 0;
}

BossAlien::~BossAlien() {
}

int BossAlien::getPhase() const { return phase; }
int BossAlien::getShield() const { return shield; }
bool BossAlien::isEntryComplete() const { return entryComplete; }

bool BossAlien::wantsExplosion() const { return explosionRequest; }
int BossAlien::getRedIntensity() const { return redLightIntensity; }
void BossAlien::setPhase(int p) { phase = p; }
int BossAlien::getAttackPattern() const { return attackPattern; }
int BossAlien::getBeamTimer() const { return beamTimer; }
int BossAlien::getBeamChargeTimer() const { return beamChargeTimer; }

void BossAlien::updateDeathAnimation() {
    tickHitFlash();

    if (get_health() == 0 && !bossDying) {
        bossDying = true;
        bossDeathTimer = 0;
    }
    if (bossDying) {
        bossDeathTimer++;
        // Explosions — frequent early, tapering off late
        if (bossDeathTimer < 80 && bossDeathTimer % 2 == 0) {
            explosionRequest = true;
        } else if (bossDeathTimer < 100 && bossDeathTimer % 5 == 0) {
            explosionRequest = true;
        }
    }
}

bool BossAlien::isDead() const {
    // Boss takes 120 frames (~1.2s) to fully die
    return bossDying && bossDeathTimer > 120;
}

void BossAlien::move(int dx, int dy) {
    moveCounter++;
    if (shieldFlash > 0) shieldFlash--;
    if (shieldBreakFlash > 0) shieldBreakFlash--;
    explosionRequest = false;  // Reset each frame
    if (beamChargeTimer > 0) {
        beamChargeTimer--;
        if (beamChargeTimer == 0) beamTimer = 30;  // Charge complete, fire beam
    }
    if (beamTimer > 0) beamTimer--;
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
    } else if (phase == 3) {
        // Swarm phase - slow drift, continuous explosions
        if (moveCounter % 30 == 0) {
            horizontalDirection *= -1;
        }
        if (moveCounter % 4 == 0) {
            explosionRequest = true;
        }
    } else {
        // Phase 4 - Erratic
        if (moveCounter % 8 == 0) {
            if (rand() % 2 == 0) {
                horizontalDirection *= -1;
            }
        }
    }

    // Bounce off edges (boss is wide, ~14px each side with fins)
    if (currentX <= 16) horizontalDirection = 1;
    if (currentX >= 47) horizontalDirection = -1;

    Alien::move(horizontalDirection, 0);  // No vertical movement after entry
}

bool BossAlien::shouldShoot() {
    if (!entryComplete || phaseTransitionTimer > 0) return false;
    if (phase == 3) return false;  // No shooting during swarm phase
    shotCooldown--;
    return shotCooldown <= 0;
}

void BossAlien::resetShotCooldown() {
    if (phase == 1) {
        shotCooldown = 35 + (rand() % 20);
    } else if (phase == 2) {
        shotCooldown = 40 + (rand() % 20);
    } else if (phase == 4) {
        // Alternate between scatter and beam
        attackPattern = 1 - attackPattern;  // Toggle 0/1
        if (attackPattern == 0) {
            shotCooldown = 20 + (rand() % 10);  // Scatter cooldown
        } else {
            shotCooldown = 50 + (rand() % 15);  // Longer pause before next attack
            beamChargeTimer = 20;  // Charge for 20 frames before firing
        }
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
    if (phase == 3) return;  // Invulnerable during swarm phase

    // Shield absorbs damage in phase 1
    if (shield > 0) {
        if (shieldFlash <= 0) shieldFlash = 3;  // Only flash if previous flash finished
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
    } else if (hp > 0 && hp <= 350 && phase == 2) {
        phase = 3;           // Swarm phase - boss invulnerable
        phaseTransitionTimer = 0;
    }
    // Phase 3->4 transition is handled by Game when swarm is cleared
}

void BossAlien::erase(RGBMatrix *matrix) {
    int x = get_x();
    int y = get_y();
    for (int dy = -14; dy <= 12; dy++) {
        for (int dx = -16; dx <= 16; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                matrix->SetPixel(py, px, 18, 10, 14);
            }
        }
    }
    // Erase beam column (3px wide, from boss to bottom)
    for (int row = y + 9; row < 64; row++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (x + dx >= 0 && x + dx < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, x + dx, 18, 10, 14);
        }
    }
}

void BossAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;

    int x = get_x();
    int y = get_y();

    if (x < 0 || x >= 64 || y < -10 || y > 70) return;

    if (get_health() == 0) {
        if (!bossDying) return;
        int dt = bossDeathTimer;
        // Final frames — actively erase everything
        if (dt > 95) {
            for (int dy = -16; dy <= 16; dy++) {
                for (int dx = -22; dx <= 22; dx++) {
                    int px = x + dx;
                    int py = y + dy;
                    if (px >= 0 && px < 64 && py >= 0 && py < 64)
                        matrix->SetPixel(py, px, 18, 10, 14);
                }
            }
            return;
        }

        // Progressive disintegration with random holes throughout
        auto sp = [matrix, dt, x, y](int py, int px, int r, int g, int b) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                // Random missing pixels increasing over time
                int missPct = dt * dt / 40;  // Accelerating: ~6% at 50, ~25% at 100
                if (missPct > 95) missPct = 95;
                if ((rand() % 100) < missPct) return;
                // Distance-based dropout
                int dist = abs(px - x) + abs(py - y);
                int threshold = dt - dist;
                if (threshold > 0 && (rand() % 6) < (threshold / 10)) return;
                // Late-stage heavy dropout
                if (dt > 60 && (rand() % 3 == 0)) return;
                if (dt > 75 && (rand() % 2 == 0)) return;
                if (dt > 85 && (rand() % 4 != 0)) return;  // ~75% gone
                // Color shifts toward red/orange then dims
                int shift = dt * 2;
                int dim = (dt > 80) ? (dt - 80) * 4 : 0;
                r = r + shift > 255 ? 255 : r + shift;
                g = g > shift ? g - shift : 0;
                b = b > shift ? b - shift : 0;
                r = r > dim ? r - dim : 0;
                g = g > dim ? g - dim : 0;
                matrix->SetPixel(py, px, r, g, b);
            }
        };
        int drift = dt / 8;  // Sections drift apart

        // Central body — diamond shape shrinking
        int cSize = 7 - dt / 18;
        if (cSize > 0) {
            for (int row = -cSize; row <= cSize; row++) {
                int hw = cSize - abs(row);  // Diamond shape
                for (int col = -hw; col <= hw; col++)
                    sp(y + row, x + col, 120, 60, 130);
            }
            // Gun remnant
            if (dt < 60) sp(y + cSize + 1, x, 160, 90, 170);
        }

        // Left wing — irregular shape drifting left and down
        if (dt < 100) {
            int lx = x - 9 - drift;
            int ly = y + drift / 3;
            for (int row = -3; row <= 2; row++) {
                int hw = 3 - abs(row) / 2;  // Tapered shape
                for (int col = -hw; col <= hw; col++)
                    sp(ly + row, lx + col, 85, 40, 95);
            }
            // Pincer fragment
            if (dt < 70) sp(ly + 3, lx - 1, 160, 90, 170);
        }

        // Right wing — mirrored, drifts right and down
        if (dt < 100) {
            int rx = x + 9 + drift;
            int ry = y + drift / 3;
            for (int row = -3; row <= 2; row++) {
                int hw = 3 - abs(row) / 2;
                for (int col = -hw; col <= hw; col++)
                    sp(ry + row, rx + col, 85, 40, 95);
            }
            if (dt < 70) sp(ry + 3, rx + 1, 160, 90, 170);
        }

        // Scattered debris particles
        if (dt > 20 && dt < 110) {
            for (int p = 0; p < 4; p++) {
                int dx = (rand() % 30) - 15;
                int dy = (rand() % 20) - 10;
                int fade = 150 - dt;
                if (fade > 0) sp(y + dy, x + dx, fade, fade / 3, fade / 2);
            }
        }
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

    if (phase <= 3) {
        // Phases 1, 2, 3 share same appearance
        cr = 160; cg = 90;  cb = 170;   // Bright plum core
        br = 120; bg = 60;  bb = 130;   // Dusty plum body
        ar = 85;  ag = 40;  ab = 95;    // Dark violet armor
        dr = 55;  dg = 28;  db = 65;    // Darkest edge
    } else {
        // Phase 4 - pulsing violet (desperate)
        int pulse = ((frameCounter / 3) % 2 == 0) ? 35 : 0;
        cr = 180 + pulse; cg = 70; cb = 190 + pulse;
        br = 140 + pulse; bg = 45; bb = 150;
        ar = 100; ag = 30; ab = 110;
        dr = 65;  dg = 20; db = 75;
    }

    // === THREE-SECTION BOSS SPRITE ===
    // Central body (head + core + gun), Left wing pod, Right wing pod
    // 1-pixel gap between sections for visual separation

    // --- CENTRAL BODY (x-4 to x+4, rows -8 to +8) ---
    // Crown
    sp(y - 8, x - 1, dr, dg, db);
    sp(y - 8, x,     ar, ag, ab);
    sp(y - 8, x + 1, dr, dg, db);
    // Head
    for (int i = -2; i <= 2; i++) sp(y - 7, x + i, ar, ag, ab);
    for (int i = -3; i <= 3; i++) sp(y - 6, x + i, br, bg, bb);
    // Core body (rows -5 to +2)
    for (int row = -5; row <= 2; row++) {
        for (int i = -4; i <= 4; i++) {
            if (phase == 4 && (rand() % 10 == 0)) continue;
            if (row == -1 && i == 0) sp(y + row, x + i, cr, cg, cb);  // CORE EYE
            else if (i >= -2 && i <= 2) sp(y + row, x + i, br, bg, bb);
            else sp(y + row, x + i, ar, ag, ab);
        }
    }
    // Lower hull narrowing
    for (int i = -3; i <= 3; i++) sp(y + 3, x + i, ar, ag, ab);
    for (int i = -2; i <= 2; i++) sp(y + 4, x + i, ar, ag, ab);
    // Gun barrel
    sp(y + 5, x - 1, ar, ag, ab);
    sp(y + 5, x,     br, bg, bb);
    sp(y + 5, x + 1, ar, ag, ab);
    sp(y + 6, x,     br, bg, bb);
    sp(y + 7, x,     cr, cg, cb);  // Gun tip

    // --- LEFT WING POD (x-13 to x-6, rows -4 to +4) ---
    // Top edge
    for (int i = -10; i <= -7; i++) sp(y - 4, x + i, dr, dg, db);
    // Main body
    for (int row = -3; row <= 2; row++) {
        for (int i = -12; i <= -6; i++) {
            if (phase == 4 && (rand() % 10 == 0)) continue;
            if (i >= -10 && i <= -8) sp(y + row, x + i, br, bg, bb);
            else if (i >= -12 && i <= -6) sp(y + row, x + i, ar, ag, ab);
        }
    }
    // Gem on wing
    sp(y - 1, x - 9, cr, cg, cb);
    // Bottom edge + pincer
    for (int i = -11; i <= -7; i++) sp(y + 3, x + i, dr, dg, db);
    sp(y + 4, x - 10, ar, ag, ab);
    sp(y + 4, x - 9, ar, ag, ab);
    sp(y + 5, x - 9, br, bg, bb);
    sp(y + 6, x - 8, cr, cg, cb);  // Left pincer tip
    // Obelisk spire above
    sp(y - 5, x - 9, dr, dg, db);
    sp(y - 6, x - 9, ar, ag, ab);
    sp(y - 7, x - 9, dr, dg, db);

    // --- RIGHT WING POD (x+6 to x+13, rows -4 to +4) ---
    // Top edge
    for (int i = 7; i <= 10; i++) sp(y - 4, x + i, dr, dg, db);
    // Main body
    for (int row = -3; row <= 2; row++) {
        for (int i = 6; i <= 12; i++) {
            if (phase == 4 && (rand() % 10 == 0)) continue;
            if (i >= 8 && i <= 10) sp(y + row, x + i, br, bg, bb);
            else sp(y + row, x + i, ar, ag, ab);
        }
    }
    // Gem on wing
    sp(y - 1, x + 9, cr, cg, cb);
    // Bottom edge + pincer
    for (int i = 7; i <= 11; i++) sp(y + 3, x + i, dr, dg, db);
    sp(y + 4, x + 10, ar, ag, ab);
    sp(y + 4, x + 9, ar, ag, ab);
    sp(y + 5, x + 9, br, bg, bb);
    sp(y + 6, x + 8, cr, cg, cb);  // Right pincer tip
    // Obelisk spire above
    sp(y - 5, x + 9, dr, dg, db);
    sp(y - 6, x + 9, ar, ag, ab);
    sp(y - 7, x + 9, dr, dg, db);

    // --- TRIANGULAR PROTRUSIONS ---
    // Central body: two side triangles pointing outward (3px wide base, 2px tall)
    // Left triangle at row -3
    sp(y - 3, x - 5, ar, ag, ab);
    sp(y - 4, x - 5, dr, dg, db); sp(y - 4, x - 6, ar, ag, ab);
    sp(y - 5, x - 6, dr, dg, db); sp(y - 5, x - 7, cr, cg, cb);  // Tip
    // Right triangle at row -3
    sp(y - 3, x + 5, ar, ag, ab);
    sp(y - 4, x + 5, dr, dg, db); sp(y - 4, x + 6, ar, ag, ab);
    sp(y - 5, x + 6, dr, dg, db); sp(y - 5, x + 7, cr, cg, cb);  // Tip

    // Wing rear triangles (pointing up-outward)
    // Left wing
    sp(y - 5, x - 11, ar, ag, ab); sp(y - 5, x - 12, dr, dg, db);
    sp(y - 6, x - 12, ar, ag, ab); sp(y - 6, x - 13, dr, dg, db);
    sp(y - 7, x - 13, cr, cg, cb);  // Tip
    // Right wing
    sp(y - 5, x + 11, ar, ag, ab); sp(y - 5, x + 12, dr, dg, db);
    sp(y - 6, x + 12, ar, ag, ab); sp(y - 6, x + 13, dr, dg, db);
    sp(y - 7, x + 13, cr, cg, cb);  // Tip

    // Wing forward triangles (pointing down, flanking pincers)
    // Left
    sp(y + 3, x - 6, ar, ag, ab);
    sp(y + 4, x - 6, dr, dg, db); sp(y + 4, x - 7, ar, ag, ab);
    sp(y + 5, x - 7, dr, dg, db); sp(y + 5, x - 8, cr, cg, cb);  // Tip
    // Right
    sp(y + 3, x + 6, ar, ag, ab);
    sp(y + 4, x + 6, dr, dg, db); sp(y + 4, x + 7, ar, ag, ab);
    sp(y + 5, x + 7, dr, dg, db); sp(y + 5, x + 8, cr, cg, cb);  // Tip

    // Lower center triangles flanking gun barrel
    sp(y + 3, x - 4, ar, ag, ab);
    sp(y + 4, x - 4, dr, dg, db); sp(y + 4, x - 3, ar, ag, ab);
    sp(y + 5, x - 3, cr, cg, cb);  // Tip
    sp(y + 3, x + 4, ar, ag, ab);
    sp(y + 4, x + 4, dr, dg, db); sp(y + 4, x + 3, ar, ag, ab);
    sp(y + 5, x + 3, cr, cg, cb);  // Tip

    // --- CONNECTING STRUTS between sections ---
    sp(y - 2, x - 5, dr, dg, db);
    sp(y - 1, x - 5, ar, ag, ab);
    sp(y,     x - 5, ar, ag, ab);
    sp(y + 1, x - 5, dr, dg, db);
    sp(y - 2, x + 5, dr, dg, db);
    sp(y - 1, x + 5, ar, ag, ab);
    sp(y,     x + 5, ar, ag, ab);
    sp(y + 1, x + 5, dr, dg, db);

    // --- TALL SIDE FINS (vertical pillars on outer wings) ---
    // Left fin at x-13, spanning rows -6 to +2
    sp(y - 7, x - 13, cr, cg, cb);  // Fin tip (bright)
    sp(y - 6, x - 13, ar, ag, ab);
    sp(y - 6, x - 14, dr, dg, db);  // Fin width
    sp(y - 5, x - 13, br, bg, bb);
    sp(y - 5, x - 14, ar, ag, ab);
    sp(y - 4, x - 13, br, bg, bb);
    sp(y - 4, x - 14, ar, ag, ab);
    sp(y - 3, x - 13, br, bg, bb);
    sp(y - 3, x - 14, dr, dg, db);
    sp(y - 2, x - 13, ar, ag, ab);
    sp(y - 2, x - 14, dr, dg, db);
    sp(y - 1, x - 13, ar, ag, ab);
    sp(y,     x - 13, dr, dg, db);
    sp(y + 1, x - 13, dr, dg, db);
    sp(y + 2, x - 13, cr, cg, cb);  // Bottom fin tip

    // Right fin at x+13 (mirrored)
    sp(y - 7, x + 13, cr, cg, cb);
    sp(y - 6, x + 13, ar, ag, ab);
    sp(y - 6, x + 14, dr, dg, db);
    sp(y - 5, x + 13, br, bg, bb);
    sp(y - 5, x + 14, ar, ag, ab);
    sp(y - 4, x + 13, br, bg, bb);
    sp(y - 4, x + 14, ar, ag, ab);
    sp(y - 3, x + 13, br, bg, bb);
    sp(y - 3, x + 14, dr, dg, db);
    sp(y - 2, x + 13, ar, ag, ab);
    sp(y - 2, x + 14, dr, dg, db);
    sp(y - 1, x + 13, ar, ag, ab);
    sp(y,     x + 13, dr, dg, db);
    sp(y + 1, x + 13, dr, dg, db);
    sp(y + 2, x + 13, cr, cg, cb);

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
        // Smooth color cycle: red -> orange -> yellow -> orange -> red
        // Uses a 60-frame cycle with linear interpolation
        int cyclePos = frameCounter % 60;  // 0-59
        int wr, wg, wb;
        int er, eg, eb;

        if (cyclePos < 20) {
            // Red (220,40,25) -> Orange (230,120,25)
            int t = cyclePos;  // 0-19
            wr = 220 + t / 2; wg = 40 + t * 4; wb = 25;
        } else if (cyclePos < 40) {
            // Orange (230,120,25) -> Yellow (240,210,40)
            int t = cyclePos - 20;  // 0-19
            wr = 230 + t / 2; wg = 120 + t * 4 + t; wb = 25 + t;
        } else {
            // Yellow (240,210,40) -> Red (220,40,25)
            int t = cyclePos - 40;  // 0-19
            wr = 240 - t; wg = 210 - t * 8 - t; wb = 40 - t;
        }
        if (wr > 255) wr = 255;
        if (wg > 255) wg = 255;
        if (wg < 25) wg = 25;
        if (wb < 15) wb = 15;
        // Edge is dimmer version
        er = wr * 3 / 4;
        eg = wg * 3 / 4;
        eb = wb * 3 / 4;

        // Weak points on each section - exposed below pincers and gun
        // Left wing weak point (x-9, y+5)
        sp(y + 4, x - 9, er, eg, eb);
        sp(y + 5, x - 10, er, eg, eb);
        sp(y + 5, x - 9, wr, wg, wb);
        sp(y + 5, x - 8, er, eg, eb);
        sp(y + 6, x - 9, er, eg, eb);

        // Center weak point below gun (x, y+6)
        sp(y + 5, x,     er, eg, eb);
        sp(y + 6, x - 1, er, eg, eb);
        sp(y + 6, x,     wr, wg, wb);
        sp(y + 6, x + 1, er, eg, eb);
        sp(y + 7, x,     er, eg, eb);

        // Right wing weak point (x+9, y+5)
        sp(y + 4, x + 9, er, eg, eb);
        sp(y + 5, x + 10, er, eg, eb);
        sp(y + 5, x + 9, wr, wg, wb);
        sp(y + 5, x + 8, er, eg, eb);
        sp(y + 6, x + 9, er, eg, eb);

        // Red weapon streaks on wings
        sp(y + 3, x - 6, 160, 30, 35);
        sp(y + 3, x + 6, 160, 30, 35);
    }

    // Shield only visible when hit or breaking — continuous line across front
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
            sr = 50; sg = 80; sb = 80 + shieldFlash * 50;
        }
        auto edge = [matrix, sr, sg, sb](int py, int px) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, sr, sg, sb);
        };
        // Shield — one connected contour across the entire front
        // Left wing outer edge, across to center, across to right wing
        // Left wing bottom
        edge(y + 4, x - 12); edge(y + 4, x - 11);
        edge(y + 5, x - 10); edge(y + 5, x - 9);
        edge(y + 6, x - 9);
        edge(y + 7, x - 8);  // Left pincer tip
        // Connect left wing to center (continuous along y+4)
        edge(y + 4, x - 10); edge(y + 4, x - 9); edge(y + 4, x - 8);
        edge(y + 4, x - 7); edge(y + 4, x - 6); edge(y + 4, x - 5);
        // Center bottom contour
        edge(y + 4, x - 4); edge(y + 4, x - 3);
        edge(y + 5, x - 2); edge(y + 5, x - 1);
        edge(y + 6, x - 1); edge(y + 6, x + 1);
        edge(y + 7, x);
        edge(y + 8, x);  // Gun tip
        edge(y + 5, x + 1); edge(y + 5, x + 2);
        edge(y + 4, x + 3); edge(y + 4, x + 4);
        // Connect center to right wing (continuous along y+4)
        edge(y + 4, x + 5); edge(y + 4, x + 6); edge(y + 4, x + 7);
        edge(y + 4, x + 8); edge(y + 4, x + 9); edge(y + 4, x + 10);
        // Right wing bottom
        edge(y + 4, x + 11); edge(y + 4, x + 12);
        edge(y + 5, x + 10); edge(y + 5, x + 9);
        edge(y + 6, x + 9);
        edge(y + 7, x + 8);  // Right pincer tip
    }
}
