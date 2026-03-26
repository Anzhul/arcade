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
    
    // Apply 5x speed during dash
    if (dashCounter > 0) {
        Alien::move(dx * 5, dy * 5);
        dashCounter--;
    } else {
        Alien::move(dx, dy);
    }
}

void FastAlien::draw(RGBMatrix *matrix) {
    if (get_health() < 0) return;  // Completely dead, don't draw
    
    int x = get_x();
    int y = get_y();
    
    // Bounds check - skip if completely off screen (allow partial at bottom)
    if (x < 0 || x >= 64 || y < 0 || y > 68) return;

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

    // Cool teal fast alien sprite - slim vertical design
    sp(y, x, 50, 150, 140);
    sp(y + 1, x, 45, 135, 125);
    sp(y - 1, x, 40, 120, 130);
    sp(y + 2, x, 60, 140, 150);
    sp(y, x - 1, 40, 90, 100);
    sp(y, x + 1, 40, 90, 100);

    // Speed trail effect
    if (dashCounter > 0) {
        sp(y - 2, x, 50, 80, 130);
        sp(y - 3, x, 35, 55, 100);
        sp(y - 4, x, 20, 30, 65);
    } else {
        sp(y - 2, x, 25, 50, 70);
    }
}
