#include "game.hpp"
#include "graphics.h"
#include <cstdlib>

Game::Game() {
    lastSpawnTime = 0;
    currentLevel = 1;
    levelDisplayTimer = 0;
    levelStartDelay = 0;
    levelActive = false;
    gameState = MAIN_MENU;
    menuSelection = 0;
    levelSelectChoice = 1;
    lastButton1 = false;
    lastJoystickY = 0;
    joystickDebounceTimer = 0;
    currentWave = 0;
    waveSpawnedBasic = 0;
    waveSpawnedFast = 0;
    waveSpawnedTank = 0;
    waveSpawnedElite = 0;
    allWavesComplete = false;

    // Initialize enemy pointers
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i] = nullptr;
    }

    // Initialize all level wave arrays to zero
    for (int i = 0; i < MAX_LEVEL; i++) {
        levels[i].numWaves = 0;
        for (int j = 0; j < MAX_WAVES; j++) {
            levels[i].waves[j] = {0, 0, 0, 0, 15};
        }
    }

    // Level 1: Easy - basic aliens with a few fast
    //                    basic, fast, tank, elite, spawnRate
    levels[0].numWaves = 2;
    levels[0].waves[0] = {15, 3, 0, 0, 15};   // Wave 1: 15 basic, 3 fast
    levels[0].waves[1] = {10, 5, 0, 0, 12};   // Wave 2: 10 basic, 5 fast

    // Level 2: Introducing tanks
    levels[1].numWaves = 2;
    levels[1].waves[0] = {10, 5, 2, 0, 12};   // Wave 1: 10 basic, 5 fast, 2 tanks
    levels[1].waves[1] = {8, 8, 3, 0, 10};    // Wave 2: 8 basic, 8 fast, 3 tanks

    // Level 3: Introducing elites
    levels[2].numWaves = 3;
    levels[2].waves[0] = {10, 5, 2, 0, 12};   // Wave 1: basics and fast
    levels[2].waves[1] = {5, 5, 3, 2, 10};    // Wave 2: mixed with elites
    levels[2].waves[2] = {5, 8, 2, 3, 8};     // Wave 3: more elites

    // Level 4: Tough
    levels[3].numWaves = 3;
    levels[3].waves[0] = {8, 10, 3, 2, 10};   // Wave 1: fast-heavy
    levels[3].waves[1] = {5, 5, 5, 5, 8};     // Wave 2: balanced tough
    levels[3].waves[2] = {3, 10, 4, 5, 7};    // Wave 3: elite and fast rush

    // Level 5: Final challenge
    levels[4].numWaves = 4;
    levels[4].waves[0] = {5, 10, 3, 3, 10};   // Wave 1: fast swarm
    levels[4].waves[1] = {3, 5, 5, 5, 8};     // Wave 2: tank wall
    levels[4].waves[2] = {5, 10, 4, 6, 7};    // Wave 3: elite assault
    levels[4].waves[3] = {3, 8, 5, 8, 6};     // Wave 4: everything

    // Level 6: Boss fight - no regular waves, boss handles minion spawning
    levels[5].numWaves = 1;
    levels[5].waves[0] = {0, 0, 0, 0, 999};

    bossActive = false;
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
    levelDisplayTimer = 100;  // Show "LEVEL 1" for 100 ticks at start
    gameState = MAIN_MENU;
    menuSelection = 0;
    levelSelectChoice = 1;
    lastButton1 = false;
    lastJoystickY = 0;
    joystickDebounceTimer = 0;
    currentWave = 0;
    waveSpawnedBasic = 0;
    waveSpawnedFast = 0;
    waveSpawnedTank = 0;
    waveSpawnedElite = 0;
    allWavesComplete = false;
    bossActive = false;
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
    } else if (gameState == VICTORY || gameState == GAME_OVER) {
        if (gameState == VICTORY) drawVictoryScreen(matrix);
        else drawGameOverScreen(matrix);

        if (levelDisplayTimer > 0) {
            levelDisplayTimer--;
            return;
        }
        // Require a fresh press of any button (must release first, then press)
        bool anyPressed = input.button1 || input.button2 || input.button3;
        bool anyWasPressed = lastButton1;  // reuse as "any button was held"
        lastButton1 = anyPressed;
        if (anyPressed && !anyWasPressed) {
            setup();
            gameState = MAIN_MENU;
            lastButton1 = true;
        }
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

    // Check if player is dead
    if (player.get_health() <= 0) {
        gameState = GAME_OVER;
        levelDisplayTimer = 25;  // Brief pause before accepting input
        return;
    }

    // Draw everything
    player.effects(input.joystick_x, input.joystick_y, clock, matrix);
    player.draw(matrix);
    player.drawBullets(matrix);
    drawEnemies(matrix);
    drawEnemyBullets(matrix);
    drawExplosions(matrix);

    // Draw HUD
    drawHUD(matrix, input.potentiometer);

    // Draw boss health bar during boss fight
    if (bossActive) {
        drawBossHealthBar(matrix);
    }

    // Draw level display if active
    drawLevelDisplay(matrix, clock);
}

void Game::drawHUD(RGBMatrix *matrix, int potentiometer) {
    // Health bar at row 0 (10 pixels wide, 1 pixel = 10 health)
    int healthPixels = player.get_health() / 10;
    for (int i = 0; i < 10; i++) {
        if (i < healthPixels) {
            matrix->SetPixel(0, i, 100, 45, 50);  // Muted dusty rose for health
        } else {
            matrix->SetPixel(0, i, 28, 14, 16);   // Dark muted for missing health
        }
    }

    // Shield/Speed capacity bar at row 2 (10 pixels wide)
    // Amber = speed capacity (no shield), Teal = shield capacity
    // potentiometer 0 = all shield (teal), 100 = all speed (amber)
    int speedCapacity = potentiometer / 10;        // 0-10 pixels for speed
    int shieldCapacity = 10 - speedCapacity;       // Remaining is shield capacity
    int currentShield = player.get_shield() / 10;  // Current shield level

    for (int i = 0; i < 10; i++) {
        if (i < shieldCapacity) {
            // Shield capacity section
            if (i < currentShield) {
                matrix->SetPixel(2, i, 45, 80, 95);   // Muted slate teal for current shield
            } else {
                matrix->SetPixel(2, i, 14, 28, 35);    // Dark teal for depleted shield
            }
        } else {
            // Speed capacity section (muted amber)
            matrix->SetPixel(2, i, 110, 90, 45);       // Muted dusty amber for speed
        }
    }
}

void Game::spawnEnemy(int clock) {
    if (currentLevel > MAX_LEVEL || !levelActive) return;
    if (currentLevel < 1 || currentLevel > MAX_LEVEL) return;

    // Boss level special handling
    if (currentLevel == 6) {
        if (allWavesComplete) return;  // Boss defeated, don't respawn
        if (!bossActive) {
            // Spawn boss
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i] == nullptr) {
                    enemies[i] = new BossAlien(32, -10);
                    bossActive = true;
                    break;
                }
            }
        } else {
            // Check if boss wants to spawn minions
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i] != nullptr && enemies[i]->getType() == BOSS && enemies[i]->get_health() > 0) {
                    BossAlien* boss = dynamic_cast<BossAlien*>(enemies[i]);
                    if (boss && boss->shouldSpawnMinion()) {
                        // Count existing minions
                        int minionCount = 0;
                        for (int k = 0; k < MAX_ENEMIES; k++) {
                            if (enemies[k] != nullptr && enemies[k]->getType() != BOSS && enemies[k]->get_health() > 0)
                                minionCount++;
                        }
                        if (minionCount < 5) {
                            for (int j = 0; j < MAX_ENEMIES; j++) {
                                if (enemies[j] == nullptr) {
                                    int rx = 10 + (rand() % 44);
                                    if (boss->getPhase() == 3) {
                                        enemies[j] = new FastAlien(rx, -8);
                                    } else {
                                        enemies[j] = new BasicAlien(rx, -8);
                                    }
                                    break;
                                }
                            }
                        }
                        boss->resetMinionCooldown();
                    }
                    break;
                }
            }
        }
        return;
    }

    if (allWavesComplete) return;  // All waves spawned, waiting for screen to clear

    LevelConfig& level = levels[currentLevel - 1];
    if (currentWave >= level.numWaves) {
        allWavesComplete = true;
        return;
    }

    Wave& wave = level.waves[currentWave];

    // Check if current wave is fully spawned
    if (waveSpawnedBasic >= wave.basicCount &&
        waveSpawnedFast >= wave.fastCount &&
        waveSpawnedTank >= wave.tankCount &&
        waveSpawnedElite >= wave.eliteCount) {
        // Advance to next wave
        currentWave++;
        waveSpawnedBasic = 0;
        waveSpawnedFast = 0;
        waveSpawnedTank = 0;
        waveSpawnedElite = 0;
        if (currentWave >= level.numWaves) {
            allWavesComplete = true;
        }
        return;
    }

    // Spawn based on wave's spawn rate
    if (clock - lastSpawnTime > wave.spawnRate) {
        // Find an empty slot
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i] == nullptr) {
                int randomX = 10 + (rand() % 44);
                int spawnY = -8;

                // Select alien type from remaining wave quota
                AlienType type = selectWaveAlienType();

                // For tanks, ensure no overlap with existing tanks
                if (type == TANK) {
                    bool tooClose = true;
                    int attempts = 0;
                    while (tooClose && attempts < 10) {
                        tooClose = false;
                        for (int k = 0; k < MAX_ENEMIES; k++) {
                            if (enemies[k] != nullptr && enemies[k]->getType() == TANK && enemies[k]->get_health() > 0) {
                                int dx = randomX - enemies[k]->get_x();
                                int dy = spawnY - enemies[k]->get_y();
                                if (dx < 0) dx = -dx;
                                if (dy < 0) dy = -dy;
                                if (dx < 10 && dy < 10) {
                                    tooClose = true;
                                    randomX = 10 + (rand() % 44);
                                    break;
                                }
                            }
                        }
                        attempts++;
                    }
                    if (tooClose) break;
                }

                // Create the appropriate alien and increment spawn counter
                switch(type) {
                    case BASIC:
                        enemies[i] = new BasicAlien(randomX, spawnY);
                        waveSpawnedBasic++;
                        break;
                    case FAST:
                        enemies[i] = new FastAlien(randomX, spawnY);
                        waveSpawnedFast++;
                        break;
                    case TANK:
                        enemies[i] = new TankAlien(randomX, spawnY);
                        waveSpawnedTank++;
                        break;
                    case ELITE:
                        enemies[i] = new EliteAlien(randomX, spawnY);
                        waveSpawnedElite++;
                        break;
                }

                lastSpawnTime = clock;
                break;
            }
        }
    }
}

void Game::updateEnemies(int clock) {
    bool anyEnemiesAlive = false;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] == nullptr) continue;

        // Check if already completely dead - clean up and skip
        if (enemies[i]->isDead()) {
            delete enemies[i];
            enemies[i] = nullptr;
            continue;
        }

        // Update death animation
        enemies[i]->updateDeathAnimation();

        // Boss death: spawn multiple explosions across its body
        if (enemies[i]->getType() == BOSS && enemies[i]->get_health() == 0) {
            if (clock % 3 == 0) {
                int ex = enemies[i]->get_x() + (rand() % 20) - 10;
                int ey = enemies[i]->get_y() + (rand() % 14) - 7;
                addExplosion(ex, ey);
            }
        }

        // Only update living enemies
        if (enemies[i]->get_health() > 0) {
            anyEnemiesAlive = true;

            // Move based on enemy type and speed
            int moveFrequency = 3;  // Default
            if (enemies[i]->getType() == BASIC) {
                moveFrequency = 2;  // BASIC enemies move every 2 frames (faster)
            } else if (enemies[i]->getType() == FAST) {
                moveFrequency = 2;  // FAST enemies move every 2 frames
            } else if (enemies[i]->getType() == TANK) {
                moveFrequency = 3;  // TANK enemies move every 3 frames
            } else if (enemies[i]->getType() == ELITE) {
                moveFrequency = 2;  // ELITE enemies move every 2 frames
            } else if (enemies[i]->getType() == BOSS) {
                BossAlien* boss = dynamic_cast<BossAlien*>(enemies[i]);
                if (boss) {
                    if (boss->getPhase() == 1) moveFrequency = 3;
                    else if (boss->getPhase() == 2) moveFrequency = 2;
                    else moveFrequency = 1;
                }
            }

            if (clock % moveFrequency == 0) {
                enemies[i]->move(0, 1);
            }

            // Boss never scrolls off — check for transition explosions
            if (enemies[i]->getType() == BOSS) {
                BossAlien* boss = dynamic_cast<BossAlien*>(enemies[i]);
                if (boss && boss->wantsExplosion()) {
                    // Spawn explosion at random position on boss body
                    int ex = boss->get_x() + (rand() % 16) - 8;
                    int ey = boss->get_y() + (rand() % 10) - 5;
                    addExplosion(ex, ey);
                }
                continue;
            }

            // Remove enemy only when fully scrolled off bottom
            // Tank extends +3 below center, others +2 max
            int bottomMargin = (enemies[i]->getType() == TANK) ? 4 : 3;
            if (enemies[i]->get_y() > 63 + bottomMargin) {
                delete enemies[i];
                enemies[i] = nullptr;
            }
        } else {
            // Health is 0, still in death animation
            anyEnemiesAlive = true;
        }
    }

    // Boss fight: check if boss is dead
    if (currentLevel == 6) {
        if (bossActive) {
            bool bossExists = false;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i] != nullptr && enemies[i]->getType() == BOSS) {
                    bossExists = true;
                    break;
                }
            }
            if (!bossExists) {
                bossActive = false;
                allWavesComplete = true;  // Prevent respawning
            }
        }
        // Once boss is gone, wait for all minions to clear
        if (!bossActive && allWavesComplete && !anyEnemiesAlive) {
            gameState = VICTORY;
            levelDisplayTimer = 25;  // Brief pause before accepting input
        }
        return;
    }

    // Level complete when all waves spawned and no enemies remain
    if (allWavesComplete && !anyEnemiesAlive) {
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
            
            // Check collision (bullet within range of enemy center)
            int dx = bx - ex;
            int dy = by - ey;
            int hitRangeX = (enemies[j]->getType() == BOSS) ? 13 : 2;
            int hitRangeY = (enemies[j]->getType() == BOSS) ? 10 : 2;

            if (dx >= -hitRangeX && dx <= hitRangeX && dy >= -hitRangeY && dy <= hitRangeY) {
                // Boss weak point check: after shield is down, only weak points take damage
                if (enemies[j]->getType() == BOSS) {
                    BossAlien* boss = dynamic_cast<BossAlien*>(enemies[j]);
                    if (boss && boss->getShield() <= 0) {
                        // Check if bullet hits a weak point (3 points with radius 3)
                        // Left: (ex-7, ey+3), Center: (ex, ey+7), Right: (ex+7, ey+3)
                        bool hitWeak = false;
                        int wpRadius = 3;
                        if (abs(bx - (ex - 7)) <= wpRadius && abs(by - (ey + 3)) <= wpRadius) hitWeak = true;
                        if (abs(bx - ex) <= wpRadius && abs(by - (ey + 7)) <= wpRadius) hitWeak = true;
                        if (abs(bx - (ex + 7)) <= wpRadius && abs(by - (ey + 3)) <= wpRadius) hitWeak = true;
                        if (!hitWeak) {
                            bullets[i].deactivate();  // Bullet absorbed by armor
                            break;
                        }
                    }
                }
                // Weak point hits deal double damage
                bool isBossWeak = (enemies[j]->getType() == BOSS && shield <= 0);
                // Need to check shield via cast
                if (enemies[j]->getType() == BOSS) {
                    BossAlien* bcheck = dynamic_cast<BossAlien*>(enemies[j]);
                    isBossWeak = (bcheck && bcheck->getShield() <= 0);
                }
                int dmgMult = isBossWeak ? 2 : 1;

                if (isRocket) {
                    enemies[j]->takeDamage(30 * dmgMult);
                    addExplosion(bx, by);

                    for (int k = 0; k < MAX_ENEMIES; k++) {
                        if (k == j || enemies[k] == nullptr || enemies[k]->get_health() <= 0) continue;

                        int ekx = enemies[k]->get_x();
                        int eky = enemies[k]->get_y();
                        int distX = ekx - bx;
                        int distY = eky - by;

                        if (distX >= -6 && distX <= 6 && distY >= -6 && distY <= 6) {
                            enemies[k]->takeDamage(15);
                        }
                    }
                } else if (isScatter) {
                    enemies[j]->takeDamage(8 * dmgMult);
                } else {
                    enemies[j]->takeDamage(5 * dmgMult);
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
            
            // Check if enemy is in laser beam
            int dx = ex - laserX;
            int laserHitRange = (enemies[j]->getType() == BOSS) ? 10 : 1;

            if (dx >= -laserHitRange && dx <= laserHitRange && ey < laserY && ey >= 0) {
                // Boss weak point check for laser
                if (enemies[j]->getType() == BOSS) {
                    BossAlien* boss = dynamic_cast<BossAlien*>(enemies[j]);
                    if (boss && boss->getShield() <= 0) {
                        // Laser must hit near a weak point x-position
                        bool hitWeak = false;
                        if (abs(laserX - (ex - 7)) <= 3) hitWeak = true;
                        if (abs(laserX - ex) <= 3) hitWeak = true;
                        if (abs(laserX - (ex + 7)) <= 3) hitWeak = true;
                        if (!hitWeak) continue;
                    }
                }
                // Laser hits! Deal damage per tick (double on boss weak points)
                if (clock - lastLaserDamageTick[j] >= 2) {
                    int laserDmg = 12;
                    if (enemies[j]->getType() == BOSS) {
                        BossAlien* blaser = dynamic_cast<BossAlien*>(enemies[j]);
                        if (blaser && blaser->getShield() <= 0) laserDmg = 24;
                    }
                    enemies[j]->takeDamage(laserDmg);
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
        
        // Check collision (ship within range of enemy)
        int dx = px - ex;
        int dy = py - ey;
        int shipHitRange = (enemies[j]->getType() == BOSS) ? 13 : 3;

        if (dx >= -shipHitRange && dx <= shipHitRange && dy >= -shipHitRange && dy <= shipHitRange) {
            if (enemies[j]->getType() == BOSS) {
                player.takeDamage(50);
                enemies[j]->takeDamage(20);  // Boss takes some damage from collision
            } else {
                player.takeDamage(30);
                enemies[j]->takeDamage(1000);  // Instantly destroy regular enemies
            }
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

    // Reset wave tracking
    currentWave = 0;
    waveSpawnedBasic = 0;
    waveSpawnedFast = 0;
    waveSpawnedTank = 0;
    waveSpawnedElite = 0;
    allWavesComplete = false;
    bossActive = false;

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

AlienType Game::selectWaveAlienType() {
    if (currentLevel < 1 || currentLevel > MAX_LEVEL) return BASIC;
    LevelConfig& level = levels[currentLevel - 1];
    if (currentWave >= level.numWaves) return BASIC;

    Wave& wave = level.waves[currentWave];

    // Build list of types that still need spawning
    AlienType available[4];
    int count = 0;
    if (waveSpawnedBasic < wave.basicCount) available[count++] = BASIC;
    if (waveSpawnedFast < wave.fastCount) available[count++] = FAST;
    if (waveSpawnedTank < wave.tankCount) available[count++] = TANK;
    if (waveSpawnedElite < wave.eliteCount) available[count++] = ELITE;

    if (count == 0) return BASIC;  // Shouldn't happen

    // Pick randomly from available types
    return available[rand() % count];
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
    
    // Draw "LEVEL" text centered (5 chars * 4px = 20px, center on 64px screen)
    drawText(matrix, "LEVEL", 22, 20, brightness, brightness, brightness);

    // Draw level number centered (1 char ~4px wide)
    char levelStr[2] = {(char)('0' + currentLevel), '\0'};
    drawText(matrix, levelStr, 30, 33, brightness, brightness, 0);
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
                joystickDebounceTimer = 6;  // Prevent rapid changes
            } else if (input.joystick_y > 50) {  // Down
                menuSelection = (menuSelection + 1) % 3;
                joystickDebounceTimer = 6;  // Prevent rapid changes
            }
        }
        
        // Select with button
        if (buttonPressed) {
            if (menuSelection == 0) {  // Play
                currentLevel = 1;
                levelDisplayTimer = 100;
                levelStartDelay = 150;  // 1.5 seconds at 100fps
                levelActive = false;
                setup();  // Reset game
                gameState = PLAYING;  // setup() resets to menu, so set again
            } else if (menuSelection == 1) {  // Level Select
                gameState = LEVEL_SELECT;
                joystickDebounceTimer = 8;  // Add debounce to prevent immediate selection
            } else if (menuSelection == 2) {  // Quit
                // Could handle quit - for now just return to menu
            }
        }
    } else if (gameState == LEVEL_SELECT) {
        // Navigate levels with joystick Y (with debouncing)
        if (joystickDebounceTimer == 0) {
            if (input.joystick_y < -50) {  // Up
                levelSelectChoice = (levelSelectChoice - 1);
                if (levelSelectChoice < 0) levelSelectChoice = 0;
                joystickDebounceTimer = 6;  // Prevent rapid changes
            } else if (input.joystick_y > 50) {  // Down
                levelSelectChoice = (levelSelectChoice + 1);
                if (levelSelectChoice > MAX_LEVEL) levelSelectChoice = MAX_LEVEL;
                joystickDebounceTimer = 6;  // Prevent rapid changes
            }
        }
        
        // Select with button1, back with button3 (only if not in debounce)
        if (buttonPressed && joystickDebounceTimer == 0) {
            int selectedLevel = levelSelectChoice;  // Save selected level
            setup();
            currentLevel = selectedLevel;  // Restore selected level after setup()
            levelSelectChoice = selectedLevel;  // Keep highlight correct
            levelDisplayTimer = 100;
            levelStartDelay = 150;  // 1.5 seconds at 100fps
            levelActive = false;
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
        } else if (c == 'C') {
            for (int j = 0; j < 3; j++) safeSetPixel(y, charX + j, r, g, b);
            safeSetPixel(y + 1, charX + 2, r, g, b);
            safeSetPixel(y + 2, charX + 2, r, g, b);
            safeSetPixel(y + 3, charX + 2, r, g, b);
            for (int j = 0; j < 3; j++) safeSetPixel(y + 4, charX + j, r, g, b);
        } else if (c == 'W') {
            for (int j = 0; j < 4; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 4; j++) safeSetPixel(y + j, charX, r, g, b);
            safeSetPixel(y + 3, charX + 1, r, g, b);
            safeSetPixel(y + 4, charX + 2, r, g, b);
            safeSetPixel(y + 4, charX, r, g, b);
        } else if (c == 'N') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX, r, g, b);
            safeSetPixel(y + 1, charX + 1, r, g, b);
        } else if (c == 'O') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX, r, g, b);
            safeSetPixel(y, charX + 1, r, g, b);
            safeSetPixel(y + 4, charX + 1, r, g, b);
        } else if (c == 'G') {
            for (int j = 0; j < 3; j++) safeSetPixel(y, charX + j, r, g, b);
            safeSetPixel(y + 1, charX + 2, r, g, b);
            safeSetPixel(y + 2, charX + 2, r, g, b);
            safeSetPixel(y + 2, charX, r, g, b);
            safeSetPixel(y + 3, charX + 2, r, g, b);
            safeSetPixel(y + 3, charX, r, g, b);
            for (int j = 0; j < 3; j++) safeSetPixel(y + 4, charX + j, r, g, b);
        } else if (c == 'M') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX, r, g, b);
            safeSetPixel(y, charX + 1, r, g, b);
            safeSetPixel(y + 1, charX + 1, r, g, b);
        } else if (c == 'R') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            safeSetPixel(y, charX + 1, r, g, b);
            safeSetPixel(y, charX, r, g, b);
            safeSetPixel(y + 1, charX, r, g, b);
            safeSetPixel(y + 2, charX, r, g, b);
            safeSetPixel(y + 2, charX + 1, r, g, b);
            safeSetPixel(y + 3, charX + 1, r, g, b);
            safeSetPixel(y + 4, charX, r, g, b);
        } else if (c >= '0' && c <= '6') {
            int num = c - '0';
            if (num == 0) {
                // 0: box shape
                for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
                for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX, r, g, b);
                safeSetPixel(y, charX + 1, r, g, b);
                safeSetPixel(y + 4, charX + 1, r, g, b);
            } else if (num == 1) {
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
            } else if (num == 6) {
                for (int j = 0; j < 3; j++) safeSetPixel(y, charX + j, r, g, b);
                safeSetPixel(y + 1, charX + 2, r, g, b);
                for (int j = 0; j < 3; j++) safeSetPixel(y + 2, charX + j, r, g, b);
                safeSetPixel(y + 3, charX + 2, r, g, b);
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
    
    // Draw levels on separate rows (0 = free fly, 1-5 = combat levels)
    int startX = 15;
    for (int i = 0; i <= MAX_LEVEL; i++) {
        char levelStr[2] = {(char)('0' + i), '\0'};
        int rowY = 15 + i * 7;  // Each level on its own row
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
                        // Fire fast violet bullet downward from elite position
                        enemyBullets[j] = Bullet(elite->get_x(), elite->get_y() + 3, 0, 3, 140, 50, 160);
                        elite->resetShotCooldown();
                        break;
                    }
                }
            }
        }
    }

    // Make boss shoot
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr && enemies[i]->getType() == BOSS && enemies[i]->get_health() > 0) {
            BossAlien* boss = dynamic_cast<BossAlien*>(enemies[i]);
            if (boss && boss->shouldShoot()) {
                int bx = boss->get_x();
                int by = boss->get_y();
                int bossPhase = boss->getPhase();

                if (bossPhase == 1) {
                    // Single aimed bullet
                    for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                        if (!enemyBullets[j].isActive()) {
                            enemyBullets[j] = Bullet(bx, by + 9, 0, 3, 140, 50, 160);
                            break;
                        }
                    }
                } else if (bossPhase == 2) {
                    // 3-bullet spread
                    int spawned = 0;
                    for (int j = 0; j < MAX_ENEMY_BULLETS && spawned < 3; j++) {
                        if (!enemyBullets[j].isActive()) {
                            int spreadDx = spawned - 1;  // -1, 0, 1
                            enemyBullets[j] = Bullet(bx + spreadDx * 2, by + 9, spreadDx, 2, 130, 60, 150);
                            spawned++;
                        }
                    }
                } else {
                    // Rapid single fast bullet
                    for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                        if (!enemyBullets[j].isActive()) {
                            enemyBullets[j] = Bullet(bx, by + 9, 0, 4, 180, 70, 190);
                            break;
                        }
                    }
                }
                boss->resetShotCooldown();
            }
            break;
        }
    }
}

void Game::drawEnemyBullets(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].isActive()) {
            enemyBullets[i].draw(matrix);
            // Draw elongated trail behind bullet (above, since bullets move down)
            int bx = enemyBullets[i].getX();
            int by = enemyBullets[i].getY();
            if (by - 1 >= 0 && by - 1 < 64 && bx >= 0 && bx < 64)
                matrix->SetPixel(by - 1, bx, 100, 35, 120);  // dimmer violet trail
            if (by - 2 >= 0 && by - 2 < 64 && bx >= 0 && bx < 64)
                matrix->SetPixel(by - 2, bx, 60, 20, 75);    // fading tail
        }
    }
}

void Game::eraseEnemyBullets(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].isActive()) {
            enemyBullets[i].erase(matrix);
            // Erase trail pixels too
            int bx = enemyBullets[i].getX();
            int by = enemyBullets[i].getY();
            if (by - 1 >= 0 && by - 1 < 64 && bx >= 0 && bx < 64)
                matrix->SetPixel(by - 1, bx, 18, 10, 14);
            if (by - 2 >= 0 && by - 2 < 64 && bx >= 0 && bx < 64)
                matrix->SetPixel(by - 2, bx, 18, 10, 14);
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
            // Hit! Player takes heavy damage
            player.takeDamage(20);
            enemyBullets[i].deactivate();
        }
    }
}

void Game::drawBossHealthBar(RGBMatrix *matrix) {
    // Find the boss
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr && enemies[i]->getType() == BOSS && enemies[i]->get_health() > 0) {
            BossAlien* boss = dynamic_cast<BossAlien*>(enemies[i]);
            if (!boss) return;

            int hp = boss->get_health();    // 0-600
            int sh = boss->getShield();     // 0-200
            int barWidth = 60;              // pixels wide (col 2-61)

            // Three-layer bar at rows 61, 62, 63:
            // Row 61: Shield layer (blue) - 200 max
            int shieldPx = (sh * barWidth) / 200;
            for (int col = 0; col < barWidth; col++) {
                int px = col + 2;
                if (col < shieldPx) {
                    matrix->SetPixel(61, px, 30, 60, 150);
                } else {
                    matrix->SetPixel(61, px, 10, 20, 40);
                }
            }

            // Row 62: Armor layer (plum) - shows HP above 200
            int armorHp = (hp > 200) ? hp - 200 : 0;  // 0-400
            int armorPx = (armorHp * barWidth) / 400;
            for (int col = 0; col < barWidth; col++) {
                int px = col + 2;
                if (col < armorPx) {
                    matrix->SetPixel(62, px, 120, 55, 130);
                } else {
                    matrix->SetPixel(62, px, 30, 14, 33);
                }
            }

            // Row 63: Core health layer (dark violet) - shows HP up to 200
            int coreHp = (hp > 200) ? 200 : hp;  // 0-200
            int corePx = (coreHp * barWidth) / 200;
            for (int col = 0; col < barWidth; col++) {
                int px = col + 2;
                if (col < corePx) {
                    matrix->SetPixel(63, px, 85, 35, 95);
                } else {
                    matrix->SetPixel(63, px, 22, 10, 25);
                }
            }
            return;
        }
    }
}

void Game::drawVictoryScreen(RGBMatrix *matrix) {
    // Clear screen
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            matrix->SetPixel(y, x, 18, 10, 14);
        }
    }
    // "YOU" = 3 chars = 12px, center: (64-12)/2 = 26
    // "WIN" = 3 chars = 12px, center: 26
    drawText(matrix, "YOU", 26, 22, 255, 200, 50);
    drawText(matrix, "WIN", 26, 34, 255, 200, 50);
}

void Game::drawGameOverScreen(RGBMatrix *matrix) {
    // Clear screen
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            matrix->SetPixel(y, x, 18, 10, 14);
        }
    }
    // "GAME" = 4 chars = 16px, center: (64-16)/2 = 24
    // "OVER" = 4 chars = 16px, center: 24
    drawText(matrix, "GAME", 24, 22, 180, 30, 30);
    drawText(matrix, "OVER", 24, 34, 180, 30, 30);
}