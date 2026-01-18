#include "game.hpp"

Game::Game() {

}

Game::~Game() {

}

void Game::setup() {
    player = Ship();
}

void Game::update(const InputState& input, RGBMatrix *matrix, int clock) {
    // Erase everything first
    player.erase(matrix);
    player.eraseBullets(matrix);

    // Update ship stats based on potentiometer distribution
    player.updateDistribution(input.potentiometer);
    player.regenerateShield(clock);

    // Update positions
    player.move(input.joystick_x, input.joystick_y, clock);
    player.dash(input.joystick_x, input.joystick_y, input.button1, clock, matrix);
    player.updateDashTrail(clock, matrix);
    player.fire(input.button1, input.button2, input.button3, clock);
    player.updateBullets();

    // Draw everything
    player.effects(input.joystick_x, input.joystick_y, clock, matrix);
    player.draw(matrix);
    player.drawBullets(matrix);

    // Draw HUD
    drawHUD(matrix, input.potentiometer);
}

void Game::drawHUD(RGBMatrix *matrix, int potentiometer) {
    // Health bar at row 0 (10 pixels wide, 1 pixel = 10 health)
    int healthPixels = player.get_health() / 10;
    for (int i = 0; i < 10; i++) {
        if (i < healthPixels) {
            matrix->SetPixel(0, i, 255, 50, 50);  // Red for health
        } else {
            matrix->SetPixel(0, i, 40, 10, 10);   // Dark red for missing health
        }
    }

    // Shield/Speed capacity bar at row 2 (10 pixels wide)
    // Yellow = speed capacity (no shield), Blue = shield capacity
    // potentiometer 0 = all shield (blue), 100 = all speed (yellow)
    int speedCapacity = potentiometer / 10;        // 0-10 pixels for speed
    int shieldCapacity = 10 - speedCapacity;       // Remaining is shield capacity
    int currentShield = player.get_shield() / 10;  // Current shield level

    for (int i = 0; i < 10; i++) {
        if (i < shieldCapacity) {
            // Shield capacity section
            if (i < currentShield) {
                matrix->SetPixel(2, i, 50, 100, 255);  // Bright blue for current shield
            } else {
                matrix->SetPixel(2, i, 15, 30, 80);    // Dark blue for depleted shield
            }
        } else {
            // Speed capacity section (yellow)
            matrix->SetPixel(2, i, 200, 180, 50);      // Yellow for speed
        }
    }
}