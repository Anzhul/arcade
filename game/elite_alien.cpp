#include "elite_alien.hpp"
#include <cstdlib>

EliteAlien::EliteAlien() : Alien(40, 1, 32, 5, ELITE) {
    horizontalDirection = (rand() % 2) * 2 - 1;  // Random -1 or 1
    movePattern = rand() % 3;  // 0 = zigzag, 1 = wave, 2 = erratic
    moveCounter = 0;
    shotCooldown = 30 + (rand() % 70);  // First shot after 30-100 frames
}

EliteAlien::EliteAlien(int x_in, int y_in) : Alien(40, 1, x_in, y_in, ELITE) {
    horizontalDirection = (rand() % 2) * 2 - 1;
    movePattern = rand() % 3;
    moveCounter = 0;
    shotCooldown = 30 + (rand() % 70);
}

EliteAlien::~EliteAlien() {
}

void EliteAlien::move(int dx, int dy) {
    moveCounter++;
    shotCooldown--;
    
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
    shotCooldown = 50 + (rand() % 50);  // Shoot every 50-100 frames
}

void EliteAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;  // Completely dead, don't draw
    
    int x = get_x();
    int y = get_y();
    
    // Bounds check - elite has antenna extending 2 pixels up
    if (x < 1 || x >= 63 || y < 2 || y >= 63) return;
    
    if (get_health() == 0) {
        // Use parent class death animation
        Alien::draw(matrix);
        return;
    }
    
    // Purple elite alien sprite (3x3 pixels with energy glow)
    // Center - bright purple
    matrix->SetPixel(y, x, 200, 0, 255);
    
    // Arms/wings
    matrix->SetPixel(y, x - 1, 150, 0, 200);
    matrix->SetPixel(y, x + 1, 150, 0, 200);
    
    // Bottom
    matrix->SetPixel(y + 1, x - 1, 100, 0, 150);
    matrix->SetPixel(y + 1, x, 130, 0, 180);
    matrix->SetPixel(y + 1, x + 1, 100, 0, 150);
    
    // Top with antenna/crown
    matrix->SetPixel(y - 1, x, 130, 0, 180);
    matrix->SetPixel(y - 2, x, 80, 0, 120);  // Antenna
}
