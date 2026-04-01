#include "beam_alien.hpp"
#include <cstdlib>

BeamAlien::BeamAlien() : Alien(50, 1, 32, -8, BEAM) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    moveCounter = 0;
    beamCooldown = 80 + (rand() % 40);
    chargeTimer = 0;
    beamTimer = 0;
    anchorY = 6 + (rand() % 8);  // Anchor near top: y=6-14
    anchored = false;
    beamTargetX = 0;
}

BeamAlien::BeamAlien(int x_in, int y_in) : Alien(50, 1, x_in, y_in, BEAM) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    moveCounter = 0;
    beamCooldown = 80 + (rand() % 40);
    chargeTimer = 0;
    beamTimer = 0;
    anchorY = 6 + (rand() % 8);
    anchored = false;
    beamTargetX = 0;
}

BeamAlien::~BeamAlien() {
}

int BeamAlien::getBeamX() const {
    return beamTargetX;
}

void BeamAlien::startCharge() {
    chargeTimer = 25;  // 25 frames of charging
    beamTargetX = get_x();  // Lock beam position at start of charge
}

void BeamAlien::tickBeam() {
    if (chargeTimer > 0) {
        chargeTimer--;
        beamTargetX = get_x();  // Keep beam centered on ship while charging
        if (chargeTimer == 0) {
            // Charge complete — fire beam, lock position
            beamTimer = 50;  // Beam lasts 50 frames
            beamTargetX = get_x();
        }
    } else if (beamTimer > 0) {
        beamTimer--;
        if (beamTimer == 0) {
            // Beam done — start cooldown
            beamCooldown = 16;
        }
    }
}

void BeamAlien::move(int dx, int dy) {
    moveCounter++;

    if (!anchored) {
        if (get_y() < anchorY) {
            Alien::move(0, dy);
        } else {
            anchored = true;
        }
        return;
    }

    // Don't move while charging or firing
    if (chargeTimer > 0 || beamTimer > 0) {
        beamCooldown--;  // Still tick cooldown for internal state
        return;
    }

    beamCooldown--;
    if (beamCooldown <= 0) {
        startCharge();
    }

    // Slow horizontal drift
    if (moveCounter % 30 == 0) {
        if (rand() % 3 == 0) {
            horizontalDirection *= -1;
        }
    }

    int currentX = get_x();
    if (currentX <= 4 || currentX >= 59) {
        horizontalDirection *= -1;
    }

    Alien::move(horizontalDirection, 0);
}

bool needsBeamStart(BeamAlien* beam) {
    if (!beam) return false;
    if (beam->isCharging() || beam->isFiring()) return false;
    // Check internal cooldown via move counter trick —
    // we exposed beamCooldown check through the state
    return true;
}

void BeamAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;

    int x = get_x();
    int y = get_y();

    if (x < 3 || x >= 61 || y < 2 || y > 66) return;

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

    bool charging = chargeTimer > 0;
    bool firing = beamTimer > 0;

    // Beam alien - cyan/electric blue energy ship
    // Row -2: energy prongs
    sp(y - 2, x - 2, 30, 90, 140);
    sp(y - 2, x + 2, 30, 90, 140);

    // Row -1: upper hull
    sp(y - 1, x - 1, 40, 110, 160);
    sp(y - 1, x, 50, 130, 180);
    sp(y - 1, x + 1, 40, 110, 160);

    // Row 0: main body
    sp(y, x - 2, 25, 75, 120);
    sp(y, x - 1, 45, 120, 170);
    sp(y, x + 1, 45, 120, 170);
    sp(y, x + 2, 25, 75, 120);

    // Center core — pulses when charging
    if (charging) {
        int pulse = (chargeTimer % 4 < 2) ? 255 : 140;
        sp(y, x, pulse, pulse, 255);
    } else if (firing) {
        sp(y, x, 255, 255, 255);  // Bright white while firing
    } else {
        sp(y, x, 60, 150, 200);
    }

    // Row +1: emitter — glows during charge/fire
    if (charging) {
        int glow = (25 - chargeTimer) * 10;
        sp(y + 1, x - 1, glow, glow / 2, glow);
        sp(y + 1, x, glow + 50, glow, 255);
        sp(y + 1, x + 1, glow, glow / 2, glow);
    } else if (firing) {
        int flicker = (beamTimer % 3 == 0) ? 40 : 0;
        sp(y + 1, x - 1, 180 + flicker, 100, 255);
        sp(y + 1, x, 255, 200 + flicker, 255);
        sp(y + 1, x + 1, 180 + flicker, 100, 255);
    } else {
        sp(y + 1, x - 1, 35, 85, 130);
        sp(y + 1, x, 50, 110, 160);
        sp(y + 1, x + 1, 35, 85, 130);
    }

    // Row +2: lower hull
    sp(y + 2, x, 30, 80, 120);

    // Draw the beam itself (vertical line from emitter to bottom of screen)
    if (firing) {
        int bx = beamTargetX;
        int startY = y + 3;
        for (int row = startY; row < 64; row++) {
            int flicker = (rand() % 3 == 0) ? 35 : 0;
            if (bx >= 0 && bx < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx, 120 + flicker, 180 + flicker, 255);
            // Side glow
            if (bx - 1 >= 0 && bx - 1 < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx - 1, 40 + flicker, 60 + flicker, 140);
            if (bx + 1 >= 0 && bx + 1 < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx + 1, 40 + flicker, 60 + flicker, 140);
        }
    }

    // Charging preview — faint dotted line showing where beam will fire
    if (charging && chargeTimer < 18) {
        int bx = beamTargetX;
        for (int row = y + 3; row < 64; row += 3) {
            if (bx >= 0 && bx < 64 && row >= 0 && row < 64)
                matrix->SetPixel(row, bx, 40, 60, 100);
        }
    }
}

void BeamAlien::erase(RGBMatrix *matrix) {
    int x = get_x();
    int y = get_y();

    // Erase body area (wider to cover prongs)
    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
            int px = x + j;
            int py = y + i;
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 18, 10, 14);
        }
    }

    // Erase beam column (full height below ship)
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
