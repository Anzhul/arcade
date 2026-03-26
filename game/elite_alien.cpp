#include "elite_alien.hpp"
#include <cstdlib>

EliteAlien::EliteAlien() : Alien(40, 1, 32, 5, ELITE) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    movePattern = rand() % 3;
    moveCounter = 0;
    shotCooldown = 20 + (rand() % 40);
    shield = 20;     // Half of 40 HP is shield
    shieldFlash = 0;
}

EliteAlien::EliteAlien(int x_in, int y_in) : Alien(40, 1, x_in, y_in, ELITE) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    movePattern = rand() % 3;
    moveCounter = 0;
    shotCooldown = 20 + (rand() % 40);
    shield = 20;
    shieldFlash = 0;
}

EliteAlien::~EliteAlien() {
}

void EliteAlien::move(int dx, int dy) {
    moveCounter++;
    shotCooldown--;
    if (shieldFlash > 0) shieldFlash--;
    
    // Horizontal movement based on pattern
    int horizontalMove = 0;
    
    if (movePattern == 0) {
        // Zigzag pattern - change direction every 15 frames
        if (moveCounter % 15 == 0) {
            horizontalDirection *= -1;
        }
        horizontalMove = horizontalDirection;
    } else if (movePattern == 1) {
        // Wave pattern - change direction every 20 frames
        if (moveCounter % 20 == 0) {
            horizontalDirection *= -1;
        }
        horizontalMove = horizontalDirection;
    } else {
        // Erratic pattern - random direction changes
        if (moveCounter % 10 == 0) {
            if (rand() % 3 == 0) {  // 33% chance to change
                horizontalDirection *= -1;
            }
        }
        horizontalMove = horizontalDirection;
    }
    
    // Reverse direction if hitting edges
    int currentX = get_x();
    if (currentX <= 3 || currentX >= 60) {
        horizontalDirection *= -1;
        horizontalMove = horizontalDirection;
    }
    
    // Apply movement
    Alien::move(horizontalMove, dy);
}

bool EliteAlien::shouldShoot() {
    shotCooldown--;
    if (shotCooldown <= 0) {
        return true;
    }
    return false;
}

void EliteAlien::resetShotCooldown() {
    shotCooldown = 25 + (rand() % 30);  // Shoot every 25-55 frames
}

void EliteAlien::takeDamage(int damage) {
    if (shield > 0) {
        shieldFlash = 5;  // Blue flash for 5 frames
        if (damage <= shield) {
            shield -= damage;
            return;  // Shield absorbed all damage
        } else {
            damage -= shield;
            shield = 0;
        }
    }
    // Pass remaining damage to base class
    Alien::takeDamage(damage);
}

int EliteAlien::getShield() const {
    return shield;
}

void EliteAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;  // Completely dead, don't draw
    
    int x = get_x();
    int y = get_y();
    
    // Bounds check - skip if completely off screen (allow partial at bottom)
    if (x < 3 || x >= 61 || y < 3 || y > 68) return;

    if (get_health() == 0) {
        Alien::draw(matrix);
        return;
    }

    // Hit flash (white blend when taking health damage)
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

    // Shield edge glow - blue outline when shield is hit
    if (shieldFlash > 0) {
        int sb = 80 + shieldFlash * 35;  // Bright blue that fades
        auto edge = [matrix, sb](int py, int px) {
            if (px >= 0 && px < 64 && py >= 0 && py < 64)
                matrix->SetPixel(py, px, 30, 60, sb);
        };
        // Outline around the sprite
        edge(y + 2, x - 2);
        edge(y + 2, x + 2);
        edge(y + 3, x);  // front center edge
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
}
