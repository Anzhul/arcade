#include "anchored_elite.hpp"
#include <cstdlib>

AnchoredElite::AnchoredElite() : Alien(60, 1, 32, -8, ANCHORED_ELITE) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    movePattern = rand() % 3;
    moveCounter = 0;
    shotCooldown = 30 + (rand() % 40);
    shield = 30;
    shieldFlash = 0;
    anchorY = 6 + (rand() % 10);  // Anchor y=6-16
    anchored = false;
    hasBeam = false;
    chargeTimer = 0;
    beamTimer = 0;
    beamCooldown = 80 + (rand() % 40);
    beamTargetX = 0;
}

AnchoredElite::AnchoredElite(int x_in, int y_in) : Alien(60, 1, x_in, y_in, ANCHORED_ELITE) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    movePattern = rand() % 3;
    moveCounter = 0;
    shotCooldown = 30 + (rand() % 40);
    shield = 30;
    shieldFlash = 0;
    anchorY = 6 + (rand() % 10);
    anchored = false;
    hasBeam = false;
    chargeTimer = 0;
    beamTimer = 0;
    beamCooldown = 80 + (rand() % 40);
    beamTargetX = 0;
}

AnchoredElite::~AnchoredElite() {
}

void AnchoredElite::move(int dx, int dy) {
    moveCounter++;
    shotCooldown--;
    if (shieldFlash > 0) shieldFlash--;

    if (!anchored) {
        if (get_y() < anchorY) {
            Alien::move(0, dy);
        } else {
            anchored = true;
        }
        return;
    }

    // Don't move horizontally while firing beam
    if (chargeTimer > 0 || beamTimer > 0) return;

    // Horizontal patrol based on pattern
    int horizontalMove = 0;

    if (movePattern == 0) {
        if (moveCounter % 15 == 0) horizontalDirection *= -1;
        horizontalMove = horizontalDirection;
    } else if (movePattern == 1) {
        if (moveCounter % 20 == 0) horizontalDirection *= -1;
        horizontalMove = horizontalDirection;
    } else {
        if (moveCounter % 10 == 0) {
            if (rand() % 3 == 0) horizontalDirection *= -1;
        }
        horizontalMove = horizontalDirection;
    }

    int currentX = get_x();
    if (currentX <= 5 || currentX >= 58) {
        horizontalDirection *= -1;
        horizontalMove = horizontalDirection;
    }

    // Horizontal only — no vertical movement
    Alien::move(horizontalMove, 0);
}

bool AnchoredElite::shouldShoot() {
    if (!anchored || hasBeam) return false;
    if (shotCooldown <= 0) return true;
    return false;
}

void AnchoredElite::resetShotCooldown() {
    shotCooldown = 25 + (rand() % 30);
}

void AnchoredElite::takeDamage(int damage) {
    if (shield > 0) {
        shieldFlash = 5;
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

int AnchoredElite::getShield() const {
    return shield;
}

void AnchoredElite::tickBeam() {
    if (!hasBeam || !anchored) return;

    if (chargeTimer > 0) {
        chargeTimer--;
        if (chargeTimer == 0) {
            beamTimer = 45;  // Beam lasts 45 frames
        }
    } else if (beamTimer > 0) {
        beamTimer--;
        if (beamTimer == 0) {
            beamCooldown = 70 + (rand() % 40);
        }
    } else {
        beamCooldown--;
        if (beamCooldown <= 0) {
            chargeTimer = 20;
            beamTargetX = get_x();
        }
    }
}

void AnchoredElite::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;

    int x = get_x();
    int y = get_y();

    if (x < 3 || x >= 61 || y < 3 || y > 68) return;

    if (get_health() == 0) {
        Alien::draw(matrix);
        return;
    }

    int flash = getHitFlash();
    bool charging = chargeTimer > 0;
    bool firing = beamTimer > 0;

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

    // Same elite sprite — dusty plum/violet palette
    // Row +2: front mandibles
    sp(y + 2, x - 1, 130, 60, 140);
    sp(y + 2, x + 1, 130, 60, 140);

    // Row +1: wide jaw / front shell
    sp(y + 1, x - 2, 85, 40, 95);
    sp(y + 1, x - 1, 110, 55, 120);
    sp(y + 1, x + 1, 110, 55, 120);
    sp(y + 1, x + 2, 85, 40, 95);

    // Center of row +1: emitter glow for beam mode
    if (hasBeam && charging) {
        int glow = (20 - chargeTimer) * 12;
        sp(y + 1, x, glow, glow / 2, 255);
    } else if (hasBeam && firing) {
        sp(y + 1, x, 255, 200, 255);
    } else {
        sp(y + 1, x, 120, 70, 130);
    }

    // Row 0: center body with wide wings
    sp(y, x - 3, 60, 35, 70);
    sp(y, x - 2, 90, 45, 100);
    sp(y, x - 1, 110, 60, 120);
    sp(y, x, 140, 80, 150);     // bright center eye
    sp(y, x + 1, 110, 60, 120);
    sp(y, x + 2, 90, 45, 100);
    sp(y, x + 3, 60, 35, 70);

    // Row -1: upper body with shoulder spikes
    sp(y - 1, x - 3, 70, 40, 80);
    sp(y - 1, x - 1, 95, 50, 105);
    sp(y - 1, x, 105, 60, 115);
    sp(y - 1, x + 1, 95, 50, 105);
    sp(y - 1, x + 3, 70, 40, 80);

    // Row -2: crown / horns
    sp(y - 2, x - 2, 80, 45, 90);
    sp(y - 2, x, 75, 40, 85);
    sp(y - 2, x + 2, 80, 45, 90);

    // Row -3: horn tips
    sp(y - 3, x - 2, 100, 55, 110);
    sp(y - 3, x + 2, 100, 55, 110);

    // Shield edge glow
    if (shieldFlash > 0) {
        int sb = 80 + shieldFlash * 35;
        auto edge = [matrix, sb](int py, int px) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 30, 60, sb);
        };
        edge(y + 2, x - 2);
        edge(y + 2, x + 2);
        edge(y + 3, x);
        edge(y + 1, x - 3);
        edge(y + 1, x + 3);
        edge(y, x - 4);
        edge(y, x + 4);
        edge(y - 1, x - 4);
        edge(y - 1, x + 4);
        edge(y - 2, x - 3);
        edge(y - 2, x + 3);
        edge(y - 3, x - 3);
        edge(y - 3, x + 3);
    }

    // Draw beam (beam-mode only)
    if (firing) {
        int bx = beamTargetX;
        for (int row = y + 3; row < 64; row++) {
            int flicker = (rand() % 3 == 0) ? 35 : 0;
            if (bx >= 0 && bx < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx, 140 + flicker, 80 + flicker, 220);
            if (bx - 1 >= 0 && bx - 1 < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx - 1, 60 + flicker, 30 + flicker, 120);
            if (bx + 1 >= 0 && bx + 1 < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx + 1, 60 + flicker, 30 + flicker, 120);
        }
    }

    // Charge preview — purple dotted line
    if (charging && chargeTimer < 15) {
        int bx = beamTargetX;
        for (int row = y + 3; row < 64; row += 3) {
            if (bx >= 0 && bx < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx, 60, 30, 90);
        }
    }
}

void AnchoredElite::erase(RGBMatrix *matrix) {
    int x = get_x();
    int y = get_y();

    // Erase body
    for (int i = -4; i <= 4; i++) {
        for (int j = -5; j <= 5; j++) {
            int px = x + j;
            int py = y + i;
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 18, 10, 14);
        }
    }

    // Erase beam column
    if (hasBeam) {
        int bx = beamTargetX;
        for (int row = y + 3; row < 64; row++) {
            if (bx - 1 >= 0 && bx - 1 < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx - 1, 18, 10, 14);
            if (bx >= 0 && bx < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx, 18, 10, 14);
            if (bx + 1 >= 0 && bx + 1 < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx + 1, 18, 10, 14);
        }
    }
}
