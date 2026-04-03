#include "elite_alien.hpp"
#include <cstdlib>

EliteAlien::EliteAlien() : Alien(40, 1, 32, 5, ELITE) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    movePattern = rand() % 3;
    moveCounter = 0;
    shotCooldown = 20 + (rand() % 40);
    shield = 30;
    shieldFlash = 0;
    behaviorMode = 0;  // Default: anchor-dash
    anchorY = 8 + (rand() % 10);
    anchored = false;
    shotsFired = 0;
    shotsBeforeDash = 20;
    charging = false;
    chargeTimer = 0;
    dashing = false;
}

EliteAlien::EliteAlien(int x_in, int y_in) : Alien(40, 1, x_in, y_in, ELITE) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    movePattern = rand() % 3;
    moveCounter = 0;
    shotCooldown = 20 + (rand() % 40);
    shield = 30;
    shieldFlash = 0;
    behaviorMode = 0;
    anchorY = 8 + (rand() % 10);
    anchored = false;
    shotsFired = 0;
    shotsBeforeDash = 5 + (rand() % 4);
    charging = false;
    chargeTimer = 0;
    dashing = false;
}

EliteAlien::~EliteAlien() {
}

void EliteAlien::move(int dx, int dy) {
    moveCounter++;
    shotCooldown--;
    if (shieldFlash > 0) shieldFlash--;

    int currentX = get_x();

    if (behaviorMode == 1) {
        // STRAIGHT: move straight down, no horizontal movement
        Alien::move(0, dy);
    } else if (behaviorMode == 2) {
        // DODGING: zigzag left/right while descending
        if (moveCounter % 10 == 0) {
            horizontalDirection *= -1;
        }
        if (currentX <= 5 || currentX >= 58) {
            horizontalDirection *= -1;
        }
        Alien::move(horizontalDirection, dy);
    } else {
        // DEFAULT: anchor at top, shoot 5-8 times, charge, then dash down

        if (dashing) {
            // Very fast dash downward
            Alien::move(0, 7);
            return;
        }

        if (charging) {
            chargeTimer--;
            if (chargeTimer <= 0) {
                dashing = true;
                charging = false;
            }
            // Hold still while charging — engines signal the dash
            return;
        }

        if (!anchored) {
            // Descend to anchor point
            if (get_y() < anchorY) {
                Alien::move(0, dy);
            } else {
                anchored = true;
            }
            return;
        }

        // Anchored: patrol horizontally
        if (moveCounter % 15 == 0) {
            if (rand() % 2 == 0) horizontalDirection *= -1;
        }
        if (currentX <= 5 || currentX >= 58) {
            horizontalDirection *= -1;
        }
        Alien::move(horizontalDirection, 0);

        // Check if enough shots fired
        if (shotsFired >= shotsBeforeDash) {
            charging = true;
            chargeTimer = 10;  // Quick charge before dash
        }
    }
}

bool EliteAlien::shouldShoot() {
    shotCooldown--;
    if (shotCooldown <= 0) {
        // Default mode: only shoot while anchored, not charging/dashing
        if (behaviorMode == 0 && (!anchored || charging || dashing)) return false;
        return true;
    }
    return false;
}

void EliteAlien::resetShotCooldown() {
    if (behaviorMode == 0) {
        shotCooldown = 15 + (rand() % 15);  // Faster while anchored
        shotsFired++;
    } else {
        shotCooldown = 25 + (rand() % 30);
    }
}

void EliteAlien::takeDamage(int damage) {
    if (shield > 0) {
        shieldFlash = 5;
        if (damage <= shield) {
            shield -= damage;
            return;
        } else {
            damage -= shield;
            shield = 0;
            // Shield broken — trigger dash in default mode
            if (behaviorMode == 0 && anchored && !charging && !dashing) {
                charging = true;
                chargeTimer = 10;
            }
        }
    }
    Alien::takeDamage(damage);
}

int EliteAlien::getShield() const {
    return shield;
}

void EliteAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;

    int x = get_x();
    int y = get_y();

    if (x < 3 || x >= 61 || y < 3 || y > 68) return;

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

    // Winged elite alien - dusty plum/violet palette
    // Row +2: front mandibles
    sp(y + 2, x - 1, 130, 60, 140);
    sp(y + 2, x + 1, 130, 60, 140);

    // Row +1: wide jaw / front shell
    sp(y + 1, x - 2, 85, 40, 95);
    sp(y + 1, x - 1, 110, 55, 120);
    sp(y + 1, x, 120, 70, 130);
    sp(y + 1, x + 1, 110, 55, 120);
    sp(y + 1, x + 2, 85, 40, 95);

    // Row 0: center body with wide wings
    sp(y, x - 3, 60, 35, 70);
    sp(y, x - 2, 90, 45, 100);
    sp(y, x - 1, 110, 60, 120);
    sp(y, x + 1, 110, 60, 120);
    sp(y, x + 2, 90, 45, 100);
    sp(y, x + 3, 60, 35, 70);

    // Center eye — glows red while charging
    if (charging) {
        int pulse = (chargeTimer % 4 < 2) ? 220 : 160;
        sp(y, x, pulse, 40, 40);
    } else if (dashing) {
        sp(y, x, 255, 50, 50);  // Bright red during dash
    } else {
        sp(y, x, 140, 80, 150);
    }

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

    // Flickering engine + tron-like trail when dashing
    if (dashing) {
        bool flick = (moveCounter / 2) % 2 == 0;
        // Engine
        sp(y - 4, x, flick ? 255 : 180, flick ? 80 : 40, flick ? 40 : 20);
        sp(y - 4, x - 1, flick ? 200 : 120, flick ? 50 : 25, flick ? 25 : 12);
        sp(y - 4, x + 1, flick ? 200 : 120, flick ? 50 : 25, flick ? 25 : 12);
        // Tron trail — red line fading upward
        sp(y - 5, x, 200, 30, 20);
        sp(y - 6, x, 170, 25, 15);
        sp(y - 7, x, 140, 20, 12);
        sp(y - 8, x, 110, 15, 10);
        sp(y - 9, x, 80, 10, 8);
        sp(y - 10, x, 50, 8, 5);
        sp(y - 11, x, 30, 5, 3);
    } else if (charging) {
        // Rapid bright flicker ramping up — warning signal
        bool flick = (moveCounter % 2) == 0;
        int intensity = (10 - chargeTimer) * 25;  // Ramps 0->250
        sp(y - 4, x, flick ? intensity : intensity / 2, flick ? 40 : 15, flick ? 20 : 8);
        sp(y - 4, x - 1, flick ? intensity / 2 : intensity / 4, 20, 10);
        sp(y - 4, x + 1, flick ? intensity / 2 : intensity / 4, 20, 10);
    } else {
        bool flick = (moveCounter / 3) % 2 == 0;
        if (flick) {
            sp(y - 4, x, 100, 25, 90);
        } else {
            sp(y - 4, x, 35, 10, 30);
        }
    }
}

void EliteAlien::erase(RGBMatrix *matrix) {
    int x = get_x();
    int y = get_y();
    // Clear body area (-4 to +4)
    for (int i = -4; i <= 4; i++) {
        for (int j = -4; j <= 4; j++) {
            int px = x + j;
            int py = y + i;
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 18, 10, 14);
        }
    }
    // Clear trail area behind (up to y-12)
    for (int i = -12; i <= -5; i++) {
        int py = y + i;
        if (x >= 0 && x < 64 && py >= 0 && py < 64)
            matrix->SetPixel(py, x, 18, 10, 14);
    }
}
