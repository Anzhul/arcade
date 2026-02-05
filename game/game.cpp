#include "game.hpp"
#include "graphics.h"
#include <cstdlib>

Game::Game() {
    lastSpawnTime = 0;
    currentLevel = 1;
    enemiesKilled = 0;
    levelDisplayTimer = 0;
    levelStartDelay = 0;
    levelActive = false;
    gameState = MAIN_MENU;
    menuSelection = 0;
    levelSelectChoice = 1;
    lastButton1 = false;
    lastJoystickY = 0;
    joystickDebounceTimer = 0;
    
    // Initialize enemy pointers
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i] = nullptr;
    }
    
    // Level 1: Easy - Mostly basic aliens
    levels[0] = {15, 80, 20, 0, 0, 20};  // 80% basic, 20% fast, kill 20
    
    // Level 2: Introducing variety
    levels[1] = {12, 50, 30, 20, 0, 30};  // mixed with tanks, kill 30
    
    // Level 3: Getting harder
    levels[2] = {10, 40, 20, 30, 10, 40};  // introduce elites, kill 40
    
    // Level 4: Tough
    levels[3] = {8, 20, 30, 30, 20, 50};  // more elites and tanks, kill 50
    
    // Level 5: Final challenge
    levels[4] = {6, 10, 20, 40, 30, 60};  // mostly tough enemies, kill 60
}

Game::~Game() {
    // Clean up dynamically allocated enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr) {
            delete enemies[i];
            enemies[i] = nullptr;
        }
    }
}

void Game::setup() {
    player = Ship();
    // Initialize all enemies as nullptr
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr) {
            delete enemies[i];
            enemies[i] = nullptr;
        }
        lastLaserDamageTick[i] = 0;
    }
    // Initialize explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        explosions[i].active = false;
        explosions[i].timer = 0;
    }
    lastSpawnTime = 0;
    currentLevel = 1;
    enemiesKilled = 0;
    levelDisplayTimer = 100;  // Show "LEVEL 1" for 100 ticks at start
    gameState = MAIN_MENU;
    menuSelection = 0;
    levelSelectChoice = 1;
    lastButton1 = false;
    lastJoystickY = 0;
    joystickDebounceTimer = 0;
}

void Game::update(const InputState& input, RGBMatrix *matrix, int clock) {
    // Handle different game states
    if (gameState == MAIN_MENU) {
        updateMenu(input);
        drawMainMenu(matrix);
        return;
    } else if (gameState == LEVEL_SELECT) {
        updateMenu(input);
        drawLevelSelectMenu(matrix);
        return;
    } else if (gameState != PLAYING) {
        return;  // Paused or other state
    }
    
    // PLAYING state
    // Handle level start delay
    if (levelStartDelay > 0) {
        // Erase old enemies and player
        eraseEnemies(matrix);
        player.erase(matrix);
        player.eraseBullets(matrix);
        
        // Update and clean up dying enemies from previous level
        updateEnemies(clock);
        
        // Clear screen to remove artifacts
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                matrix->SetPixel(y, x, 18, 10, 14);
            }
        }
        
        levelStartDelay--;
        if (levelStartDelay == 0) {
            levelActive = true;
        }
        
        // During delay, allow player movement and actions but don't spawn enemies
        
        // Update ship stats based on potentiometer distribution
        player.updateDistribution(input.potentiometer);
        player.updateWeaponMode(input.potentiometer2);
        player.regenerateShield(clock);
        
        // Update positions
        player.move(input.joystick_x, input.joystick_y, clock);
        player.dash(input.joystick_x, input.joystick_y, input.button1, clock, matrix);
        player.updateDashTrail(clock, matrix);
        player.fire(input.button1, input.button2, input.button3, clock, matrix);
        player.updateBullets();
        player.updateLaser(clock, matrix);
        player.activateShieldBubble(input.button3, clock, matrix);
        player.updateShieldBubble(clock, matrix);
        
        // Draw everything
        player.effects(input.joystick_x, input.joystick_y, clock, matrix);
        player.draw(matrix);
        player.drawBullets(matrix);
        drawHUD(matrix, input.potentiometer);
        drawLevelDisplay(matrix, clock);
        return;
    }
    
    // Normal game logic
    // Erase everything first (before updating positions)
    player.erase(matrix);
    player.eraseBullets(matrix);
    eraseEnemies(matrix);  // Erase at current position
    eraseEnemyBullets(matrix);
    eraseExplosions(matrix);

    // Update ship stats based on potentiometer distribution
    player.updateDistribution(input.potentiometer);
    player.updateWeaponMode(input.potentiometer2);
    player.regenerateShield(clock);

    // Update positions
    player.move(input.joystick_x, input.joystick_y, clock);
    player.dash(input.joystick_x, input.joystick_y, input.button1, clock, matrix);
    player.updateDashTrail(clock, matrix);
    player.fire(input.button1, input.button2, input.button3, clock, matrix);
    player.updateBullets();
    player.updateLaser(clock, matrix);
    player.activateShieldBubble(input.button3, clock, matrix);
    player.updateShieldBubble(clock, matrix);

    // Update enemies (after erasing at old positions)
    spawnEnemy(clock);
    updateEnemies(clock);
    updateEnemyBullets();
    updateExplosions();
    
    // Check collisions between bullets and enemies
    checkCollisions(clock);
    checkEnemyBulletCollisions();

    // Draw everything
    player.effects(input.joystick_x, input.joystick_y, clock, matrix);
    player.draw(matrix);
    player.drawBullets(matrix);
    drawEnemies(matrix);
    drawEnemyBullets(matrix);
    drawExplosions(matrix);

    // Draw HUD
    drawHUD(matrix, input.potentiometer);
    
    // Draw level display if active
    drawLevelDisplay(matrix, clock);
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

void Game::spawnEnemy(int clock) {
    if (currentLevel > MAX_LEVEL || !levelActive) return;  // No spawning after max level or during delay
    if (currentLevel < 1 || currentLevel > MAX_LEVEL) return;  // Safety check
    
    LevelConfig& level = levels[currentLevel - 1];
    
    // Spawn based on level's spawn rate
    if (clock - lastSpawnTime > level.spawnRate) {
        // Find an empty slot
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i] == nullptr) {
                // Spawn at random x position at top of screen (off-screen so they enter gradually)
                int randomX = 10 + (rand() % 44);  // Keep enemies away from edges
                int spawnY = -8;  // Start off-screen at the top
                
                // Select alien type based on level configuration
                AlienType type = selectAlienType();
                
                // Create the appropriate alien subclass
                switch(type) {
                    case BASIC:
                        enemies[i] = new BasicAlien(randomX, spawnY);
                        break;
                    case FAST:
                        enemies[i] = new FastAlien(randomX, spawnY);
                        break;
                    case TANK:
                        enemies[i] = new TankAlien(randomX, spawnY);
                        break;
                    case ELITE:
                        enemies[i] = new EliteAlien(randomX, spawnY);
                        break;
                }
                
                lastSpawnTime = clock;
                break;
            }
        }
    }
}

void Game::updateEnemies(int clock) {
    bool shouldAdvanceLevel = false;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] == nullptr) continue;
        
        // Check if already completely dead - clean up and skip
        if (enemies[i]->isDead()) {
            delete enemies[i];
            enemies[i] = nullptr;
            continue;
        }
        
        // Check if enemy just died this frame (health went to 0)
        bool wasDying = enemies[i]->get_health() == 0;
        
        // Update death animation
        enemies[i]->updateDeathAnimation();
        
        // If enemy just became completely dead, count the kill (cleanup will happen next frame)
        if (wasDying && enemies[i]->isDead()) {
            enemiesKilled++;
            
            // Check if level is complete
            if (currentLevel > 0 && currentLevel <= MAX_LEVEL) {
                LevelConfig& level = levels[currentLevel - 1];
                if (enemiesKilled >= level.enemiesToKill) {
                    shouldAdvanceLevel = true;  // Set flag instead of calling immediately
                }
            }
        }
        
        // Only update living enemies
        if (enemies[i]->get_health() > 0) {
            // Move based on enemy type and speed
            int moveFrequency = 3;  // Default
            if (enemies[i]->getType() == FAST) {
                moveFrequency = 2;  // FAST enemies move every 2 frames
            } else if (enemies[i]->getType() == TANK) {
                moveFrequency = 5;  // TANK enemies move every 5 frames (slower)
            }
            
            if (clock % moveFrequency == 0) {
                enemies[i]->move(0, 1);
            }
            
            // Remove enemy if it goes off screen (64 pixel high screen)
            if (enemies[i]->get_y() > 64) {
                enemies[i]->setHealth(0);
            }
        }
    }
    
    // Advance level after loop completes to avoid use-after-free
    if (shouldAdvanceLevel) {
        advanceLevel();
    }
}

void Game::drawEnemies(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr) {
            // Additional safety: check if object is valid before calling draw
            try {
                enemies[i]->draw(matrix);
            } catch (...) {
                // If draw crashes, clean up the enemy
                delete enemies[i];
                enemies[i] = nullptr;
            }
        }
    }
}

void Game::eraseEnemies(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr) {
            // Additional safety: check if object is valid before calling erase
            try {
                enemies[i]->erase(matrix);
            } catch (...) {
                // If erase crashes, clean up the enemy
                delete enemies[i];
                enemies[i] = nullptr;
            }
        }
    }
}

void Game::checkCollisions(int clock) {
    Bullet* bullets = player.getBullets();
    
    // Check bullet-enemy collisions
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].isActive()) continue;
        
        int bx = bullets[i].getX();
        int by = bullets[i].getY();
        bool isRocket = bullets[i].getRocket();
        bool isScatter = bullets[i].getScatter();
        
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (enemies[j] == nullptr || enemies[j]->get_health() <= 0) continue;
            
            int ex = enemies[j]->get_x();
            int ey = enemies[j]->get_y();
            
            // Check collision (bullet within 2 pixels of enemy center)
            int dx = bx - ex;
            int dy = by - ey;
            
            if (dx >= -2 && dx <= 2 && dy >= -2 && dy <= 2) {
                if (isRocket) {
                    // Rocket explosion! Deal 30 damage to hit enemy and 15 to nearby enemies
                    enemies[j]->takeDamage(30);
                    addExplosion(bx, by);  // Create visual explosion
                    
                    // Area of effect damage (6 pixel radius)
                    for (int k = 0; k < MAX_ENEMIES; k++) {
                        if (k == j || enemies[k] == nullptr || enemies[k]->get_health() <= 0) continue;
                        
                        int ekx = enemies[k]->get_x();
                        int eky = enemies[k]->get_y();
                        int distX = ekx - bx;
                        int distY = eky - by;
                        
                        // Check if within explosion radius (6 pixels)
                        if (distX >= -6 && distX <= 6 && distY >= -6 && distY <= 6) {
                            enemies[k]->takeDamage(15);  // AoE damage
                        }
                    }
                } else if (isScatter) {
                    // Scatter shot: 8 damage per bullet
                    enemies[j]->takeDamage(8);
                } else {
                    // Normal bullet: 5 damage
                    enemies[j]->takeDamage(5);
                }
                bullets[i].deactivate();
                break;  // Bullet can only hit one enemy directly
            }
        }
    }
    
    // Check laser-enemy collisions
    if (player.getLaserActive() > 0) {
        int laserX = player.getLastLaserX();
        int laserY = player.getLastLaserY();
        
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (enemies[j] == nullptr || enemies[j]->get_health() <= 0) continue;
            
            int ex = enemies[j]->get_x();
            int ey = enemies[j]->get_y();
            
            // Check if enemy is in laser beam (x within 1 pixel, y above laser start)
            int dx = ex - laserX;
            
            if (dx >= -1 && dx <= 1 && ey < laserY && ey >= 0) {
                // Laser hits! Deal 12 damage per tick (but only once every 2 ticks per enemy)
                if (clock - lastLaserDamageTick[j] >= 2) {
                    enemies[j]->takeDamage(12);
                    lastLaserDamageTick[j] = clock;
                }
            }
        }
    }
    
    // Check ship-enemy collisions
    int px = player.get_x();
    int py = player.get_y();
    
    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (enemies[j] == nullptr || enemies[j]->get_health() <= 0) continue;
        
        int ex = enemies[j]->get_x();
        int ey = enemies[j]->get_y();
        
        // Check collision (ship within 3 pixels of enemy)
        int dx = px - ex;
        int dy = py - ey;
        
        if (dx >= -3 && dx <= 3 && dy >= -3 && dy <= 3) {
            // Collision! Ship takes heavy damage, enemy is destroyed
            player.takeDamage(30);
            enemies[j]->takeDamage(1000);  // Instantly destroy the enemy
        }
    }
}

void Game::addExplosion(int x, int y) {
    // Find an inactive explosion slot
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) {
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].timer = 0;
            explosions[i].active = true;
            break;
        }
    }
}

void Game::updateExplosions() {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (explosions[i].active) {
            explosions[i].timer++;
            if (explosions[i].timer > 12) {  // Explosion lasts 12 frames
                explosions[i].active = false;
            }
        }
    }
}

void Game::drawExplosions(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) continue;
        
        int x = explosions[i].x;
        int y = explosions[i].y;
        int timer = explosions[i].timer;
        
        // Expanding ring explosion with fade
        int fade = 255 - (timer * 21);  // Fade over 12 frames
        int size = timer / 2;  // Expand slowly
        
        // Draw explosion center - bright red-orange
        matrix->SetPixel(y, x, fade, fade / 2, 0);
        
        // Draw expanding ring
        if (size > 0) {
            // Cardinal directions
            matrix->SetPixel(y - size, x, fade / 2, fade / 4, 0);
            matrix->SetPixel(y + size, x, fade / 2, fade / 4, 0);
            matrix->SetPixel(y, x - size, fade / 2, fade / 4, 0);
            matrix->SetPixel(y, x + size, fade / 2, fade / 4, 0);
        }
        
        if (size > 1) {
            // Diagonal ring
            matrix->SetPixel(y - size, x - size, fade / 3, fade / 6, 0);
            matrix->SetPixel(y - size, x + size, fade / 3, fade / 6, 0);
            matrix->SetPixel(y + size, x - size, fade / 3, fade / 6, 0);
            matrix->SetPixel(y + size, x + size, fade / 3, fade / 6, 0);
        }
        
        // Inner bright core for first few frames
        if (timer < 6) {
            matrix->SetPixel(y - 1, x, fade, fade / 3, 0);
            matrix->SetPixel(y + 1, x, fade, fade / 3, 0);
            matrix->SetPixel(y, x - 1, fade, fade / 3, 0);
            matrix->SetPixel(y, x + 1, fade, fade / 3, 0);
        }
    }
}

void Game::eraseExplosions(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) continue;
        
        int x = explosions[i].x;
        int y = explosions[i].y;
        
        // Erase 13x13 area to cover full explosion
        for (int dy = -6; dy <= 6; dy++) {
            for (int dx = -6; dx <= 6; dx++) {
                matrix->SetPixel(y + dy, x + dx, 18, 10, 14);
            }
        }
    }
}

void Game::advanceLevel() {
    currentLevel++;
    
    // Safety clamp
    if (currentLevel < 1) currentLevel = 1;
    if (currentLevel > MAX_LEVEL) currentLevel = MAX_LEVEL;
    
    enemiesKilled = 0;
    levelDisplayTimer = 100;  // Show new level for 100 ticks
    levelStartDelay = 150;  // 1.5 second delay before starting
    levelActive = false;  // Stop spawning during delay
    
    // Immediately delete all enemies when advancing level
    // It's safe now because we're setting a flag to avoid calling this during updateEnemies loop
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr) {
            delete enemies[i];
            enemies[i] = nullptr;
        }
    }
}

AlienType Game::selectAlienType() {
    if (currentLevel < 1 || currentLevel > MAX_LEVEL) return BASIC;  // Safety fallback
    
    LevelConfig& level = levels[currentLevel - 1];
    int roll = rand() % 100;
    
    if (roll < level.basicPercent) {
        return BASIC;
    } else if (roll < level.basicPercent + level.fastPercent) {
        return FAST;
    } else if (roll < level.basicPercent + level.fastPercent + level.tankPercent) {
        return TANK;
    } else {
        return ELITE;
    }
}

void Game::drawLevelDisplay(RGBMatrix *matrix, int clock) {
    if (levelDisplayTimer <= 0) return;
    
    levelDisplayTimer--;
    
    // Fade in/out effect
    int brightness = 255;
    if (levelDisplayTimer > 80) {
        brightness = (100 - levelDisplayTimer) * 12;  // Fade in
    } else if (levelDisplayTimer < 20) {
        brightness = levelDisplayTimer * 12;  // Fade out
    }
    
    // Draw "LEVEL" text vertically
    drawText(matrix, "LEVEL", 28, 15, brightness, brightness, brightness);
    
    // Draw level number
    char levelStr[2] = {(char)('0' + currentLevel), '\0'};
    drawText(matrix, levelStr, 35, 28, brightness, brightness, 0);
}

void Game::updateMenu(const InputState& input) {
    // Detect button press (only trigger on press, not hold)
    bool button1Pressed = input.button1 && !lastButton1;
    lastButton1 = input.button1;
    bool buttonPressed = button1Pressed || input.button2;  // Either button can select
    
    // Decrement debounce timer
    if (joystickDebounceTimer > 0) {
        joystickDebounceTimer--;
    }
    
    if (gameState == MAIN_MENU) {
        // Navigate menu with joystick Y (with debouncing)
        if (joystickDebounceTimer == 0) {
            if (input.joystick_y < -50) {  // Up
                menuSelection = (menuSelection - 1 + 3) % 3;
                joystickDebounceTimer = 15;  // Prevent rapid changes
            } else if (input.joystick_y > 50) {  // Down
                menuSelection = (menuSelection + 1) % 3;
                joystickDebounceTimer = 15;  // Prevent rapid changes
            }
        }
        
        // Select with button
        if (buttonPressed) {
            if (menuSelection == 0) {  // Play
                currentLevel = 1;
                enemiesKilled = 0;
                levelDisplayTimer = 100;
                levelStartDelay = 150;  // 1.5 seconds at 100fps
                levelActive = false;
                setup();  // Reset game
                gameState = PLAYING;  // setup() resets to menu, so set again
            } else if (menuSelection == 1) {  // Level Select
                gameState = LEVEL_SELECT;
                joystickDebounceTimer = 20;  // Add debounce to prevent immediate selection
            } else if (menuSelection == 2) {  // Quit
                // Could handle quit - for now just return to menu
            }
        }
    } else if (gameState == LEVEL_SELECT) {
        // Navigate levels with joystick Y (with debouncing)
        if (joystickDebounceTimer == 0) {
            if (input.joystick_y < -50) {  // Up
                levelSelectChoice = (levelSelectChoice - 1);
                if (levelSelectChoice < 1) levelSelectChoice = 1;
                joystickDebounceTimer = 15;  // Prevent rapid changes
            } else if (input.joystick_y > 50) {  // Down
                levelSelectChoice = (levelSelectChoice + 1);
                if (levelSelectChoice > MAX_LEVEL) levelSelectChoice = MAX_LEVEL;
                joystickDebounceTimer = 15;  // Prevent rapid changes
            }
        }
        
        // Select with button1, back with button3 (only if not in debounce)
        if (buttonPressed && joystickDebounceTimer == 0) {
            int selectedLevel = levelSelectChoice;  // Save selected level
            enemiesKilled = 0;
            levelDisplayTimer = 100;
            levelStartDelay = 150;  // 1.5 seconds at 100fps
            levelActive = false;
            setup();
            currentLevel = selectedLevel;  // Restore selected level after setup()
            gameState = PLAYING;
        } else if (input.button3 && joystickDebounceTimer == 0) {
            gameState = MAIN_MENU;
        }
    }
}

void Game::drawText(RGBMatrix *matrix, const char* text, int x, int y, int r, int g, int b) {
    // Draw text horizontally flipped (mirrored)
    // Each character is 5 pixels tall, 3 pixels wide
    int len = 0;
    while (text[len] != '\0') len++;
    
    // Helper function to safely set pixel with bounds checking
    auto safeSetPixel = [matrix](int py, int px, int r, int g, int b) {
        if (px >= 0 && px < 64 && py >= 0 && py < 64) {
            matrix->SetPixel(py, px, r, g, b);
        }
    };
    
    // First erase the area with background color (larger area to ensure clean erase)
    int totalWidth = len * 4 + 2;  // 4 pixels per char including spacing + padding
    for (int i = -1; i < 7; i++) {  // 8 pixels tall (5 + extra padding)
        for (int j = -1; j < totalWidth + 1; j++) {
            safeSetPixel(y + i, x + j, 18, 10, 14);
        }
    }
    
    for (int i = 0; i < len; i++) {
        char c = text[i];
        int charX = x + ((len - 1 - i) * 4);  // Reverse order for horizontal flip
        
        if (c == 'P') {
            // Vertical line (flipped)
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            // Top curve (flipped)
            safeSetPixel(y, charX + 1, r, g, b);
            safeSetPixel(y, charX, r, g, b);
            safeSetPixel(y + 1, charX, r, g, b);
            safeSetPixel(y + 2, charX, r, g, b);
            safeSetPixel(y + 2, charX + 1, r, g, b);
        } else if (c == 'L') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 3; j++) safeSetPixel(y + 4, charX + j, r, g, b);
        } else if (c == 'A') {
            for (int j = 1; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 1; j < 5; j++) safeSetPixel(y + j, charX, r, g, b);
            for (int j = 0; j < 3; j++) safeSetPixel(y, charX + j, r, g, b);
            safeSetPixel(y + 2, charX + 1, r, g, b);
        } else if (c == 'Y') {
            safeSetPixel(y, charX + 2, r, g, b);
            safeSetPixel(y + 1, charX + 2, r, g, b);
            safeSetPixel(y, charX, r, g, b);
            safeSetPixel(y + 1, charX, r, g, b);
            for (int j = 2; j < 5; j++) safeSetPixel(y + j, charX + 1, r, g, b);
        } else if (c == 'E') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 3; j++) {
                safeSetPixel(y, charX + j, r, g, b);
                safeSetPixel(y + 2, charX + j, r, g, b);
                safeSetPixel(y + 4, charX + j, r, g, b);
            }
        } else if (c == 'V') {
            for (int j = 0; j < 4; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 4; j++) safeSetPixel(y + j, charX, r, g, b);
            safeSetPixel(y + 4, charX + 1, r, g, b);
        } else if (c == 'S') {
            for (int j = 0; j < 3; j++) safeSetPixel(y, charX + j, r, g, b);
            safeSetPixel(y + 1, charX + 2, r, g, b);
            for (int j = 0; j < 3; j++) safeSetPixel(y + 2, charX + j, r, g, b);
            safeSetPixel(y + 3, charX, r, g, b);
            for (int j = 0; j < 3; j++) safeSetPixel(y + 4, charX + j, r, g, b);
        } else if (c == 'T') {
            for (int j = 0; j < 3; j++) safeSetPixel(y, charX + j, r, g, b);
            for (int j = 1; j < 5; j++) safeSetPixel(y + j, charX + 1, r, g, b);
        } else if (c == 'Q') {
            for (int j = 0; j < 4; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 4; j++) safeSetPixel(y + j, charX, r, g, b);
            safeSetPixel(y, charX + 1, r, g, b);
            safeSetPixel(y + 3, charX + 1, r, g, b);
            safeSetPixel(y + 4, charX, r, g, b);
        } else if (c == 'U') {
            for (int j = 0; j < 4; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 4; j++) safeSetPixel(y + j, charX, r, g, b);
            safeSetPixel(y + 4, charX + 1, r, g, b);
        } else if (c == 'I') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 1, r, g, b);
        } else if (c >= '1' && c <= '5') {
            int num = c - '0';
            if (num == 1) {
                for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 1, r, g, b);
            } else if (num == 2) {
                for (int j = 0; j < 3; j++) safeSetPixel(y, charX + j, r, g, b);
                safeSetPixel(y + 1, charX, r, g, b);
                for (int j = 0; j < 3; j++) safeSetPixel(y + 2, charX + j, r, g, b);
                safeSetPixel(y + 3, charX + 2, r, g, b);
                for (int j = 0; j < 3; j++) safeSetPixel(y + 4, charX + j, r, g, b);
            } else if (num == 3) {
                for (int j = 0; j < 3; j++) {
                    safeSetPixel(y, charX + j, r, g, b);
                    safeSetPixel(y + 2, charX + j, r, g, b);
                    safeSetPixel(y + 4, charX + j, r, g, b);
                }
                safeSetPixel(y + 1, charX, r, g, b);
                safeSetPixel(y + 3, charX, r, g, b);
            } else if (num == 4) {
                for (int j = 0; j < 3; j++) safeSetPixel(y + j, charX + 2, r, g, b);
                safeSetPixel(y + 2, charX + 1, r, g, b);
                for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX, r, g, b);
            } else if (num == 5) {
                for (int j = 0; j < 3; j++) safeSetPixel(y, charX + j, r, g, b);
                safeSetPixel(y + 1, charX + 2, r, g, b);
                for (int j = 0; j < 3; j++) safeSetPixel(y + 2, charX + j, r, g, b);
                safeSetPixel(y + 3, charX, r, g, b);
                for (int j = 0; j < 3; j++) safeSetPixel(y + 4, charX + j, r, g, b);
            }
        }
    }
}

void Game::drawMainMenu(RGBMatrix *matrix) {
    // Clear screen
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            matrix->SetPixel(y, x, 18, 10, 14);
        }
    }
    
    // Draw simple text for menu options - each on its own row
    int startX = 10;
    
    // PLAY - row 1
    if (menuSelection == 0) {
        drawText(matrix, "PLAY", startX, 15, 255, 255, 0);
    } else {
        drawText(matrix, "PLAY", startX, 15, 150, 150, 150);
    }
    
    // LEVEL - row 2
    if (menuSelection == 1) {
        drawText(matrix, "LEVEL", startX, 25, 255, 255, 0);
    } else {
        drawText(matrix, "LEVEL", startX, 25, 150, 150, 150);
    }
    
    // QUIT - row 3
    if (menuSelection == 2) {
        drawText(matrix, "QUIT", startX, 35, 255, 255, 0);
    } else {
        drawText(matrix, "QUIT", startX, 35, 150, 150, 150);
    }
}

void Game::drawLevelSelectMenu(RGBMatrix *matrix) {
    // Clear screen
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            matrix->SetPixel(y, x, 18, 10, 14);
        }
    }
    
    // Draw title
    drawText(matrix, "SELECT", 10, 5, 255, 255, 255);
    
    // Draw levels on separate rows
    int startX = 15;
    for (int i = 1; i <= MAX_LEVEL; i++) {
        char levelStr[2] = {(char)('0' + i), '\0'};
        int rowY = 15 + (i - 1) * 8;  // Each level on its own row
        if (i == levelSelectChoice) {
            drawText(matrix, levelStr, startX, rowY, 255, 255, 0);  // Yellow for selected
        } else {
            drawText(matrix, levelStr, startX, rowY, 100, 100, 100);  // Gray for unselected
        }
    }
}

void Game::updateEnemyBullets() {
    // Update all enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].isActive()) {
            enemyBullets[i].update();
            
            // Deactivate if off screen
            if (enemyBullets[i].getY() > 63 || enemyBullets[i].getY() < 0 ||
                enemyBullets[i].getX() > 63 || enemyBullets[i].getX() < 0) {
                enemyBullets[i].deactivate();
            }
        }
    }
    
    // Make elite aliens shoot
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr && enemies[i]->getType() == ELITE && enemies[i]->get_health() > 0) {
            EliteAlien* elite = dynamic_cast<EliteAlien*>(enemies[i]);
            if (elite && elite->shouldShoot()) {
                // Find an inactive bullet slot
                for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                    if (!enemyBullets[j].isActive()) {
                        // Fire red bullet downward from elite position
                        enemyBullets[j] = Bullet(elite->get_x(), elite->get_y() + 2, 0, 2, 255, 0, 0);
                        elite->resetShotCooldown();
                        break;
                    }
                }
            }
        }
    }
}

void Game::drawEnemyBullets(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].isActive()) {
            enemyBullets[i].draw(matrix);
        }
    }
}

void Game::eraseEnemyBullets(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].isActive()) {
            enemyBullets[i].erase(matrix);
        }
    }
}

void Game::checkEnemyBulletCollisions() {
    int px = player.get_x();
    int py = player.get_y();
    
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].isActive()) continue;
        
        int bx = enemyBullets[i].getX();
        int by = enemyBullets[i].getY();
        
        // Check collision with player (within 2 pixels)
        int dx = bx - px;
        int dy = by - py;
        
        if (dx >= -2 && dx <= 2 && dy >= -2 && dy <= 2) {
            // Hit! Player takes damage
            player.takeDamage(10);
            enemyBullets[i].deactivate();
        }
    }
}