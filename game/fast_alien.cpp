#include "fast_alien.hpp"
#include <cstdlib>

FastAlien::FastAlien() : Alien(20, 2, 32, 5, FAST) {
    // Set random dash threshold between 10-20% of board (64 pixels)
    // 10% = 6 pixels, 20% = 13 pixels from spawn (y=5)
    dashThreshold = 5 + 6 + (rand() % 8);  // Between y=11 and y=18
    hasDashed = false;
    dashCounter = 0;
}

FastAlien::FastAlien(int x_in, int y_in) : Alien(20, 2, x_in, y_in, FAST) {
    dashThreshold = y_in + 6 + (rand() % 8);
    hasDashed = false;
    dashCounter = 0;
}

FastAlien::~FastAlien() {
}

void FastAlien::move(int dx, int dy) {
    int currentY = get_y();
    
    // Check if we've reached dash threshold
    if (!hasDashed && currentY >= dashThreshold) {
        hasDashed = true;
        dashCounter = 10;  // Dash for 10 frames
    }
    
    // Apply 3x speed during dash
    if (dashCounter > 0) {
        Alien::move(dx * 3, dy * 3);
        dashCounter--;
    } else {
        Alien::move(dx, dy);
    }
}

void FastAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;  // Completely dead, don't draw
    
    int x = get_x();
    int y = get_y();
    
    // Bounds check - don't draw if off screen
    if (x < 0 || x >= 64 || y < 0 || y >= 64) return;
    
    if (get_health() == 0) {
        // Use parent class death animation
        Alien::draw(matrix);
        return;
    }
    
    // Yellow fast alien sprite - slim vertical design (1-2 pixels wide)
    // Center core
    matrix->SetPixel(y, x, 255, 255, 0);
    matrix->SetPixel(y + 1, x, 255, 255, 0);
    
    // Top point
    matrix->SetPixel(y - 1, x, 200, 200, 0);
    
    // Bottom
    matrix->SetPixel(y + 2, x, 180, 180, 0);
    
    // Speed trail effect (dimmer pixels behind)
    if (dashCounter > 0) {
        // Stronger trail during dash
        matrix->SetPixel(y - 2, x, 150, 150, 0);
        matrix->SetPixel(y - 3, x, 100, 100, 0);
    } else {
        matrix->SetPixel(y - 2, x, 100, 100, 0);
    }
}
