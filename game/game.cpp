#include "game.hpp"
#include "graphics.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

Game::Game() {
    lastSpawnTime = 0;
    lastSpawnBasic = 0;
    lastSpawnFast = 0;
    lastSpawnTank = 0;
    lastSpawnElite = 0;
    lastSpawnLauncher = 0;
    lastSpawnBeam = 0;
    lastSpawnAnchoredElite = 0;
    lastSpawnAnchoredEliteBeam = 0;
    lastSpawnRocketBoss = 0;
    lastSpawnMiniBoss = 0;
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
    waveSpawnedLauncher = 0;
    waveSpawnedBeam = 0;
    waveSpawnedAnchoredElite = 0;
    waveSpawnedAnchoredEliteBeam = 0;
    waveSpawnedRocketBoss = 0;
    waveSpawnedMiniBoss = 0;
    allWavesComplete = false;
    waveDelayTimer = 0;
    waveTick = 0;

    // Initialize enemy pointers
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i] = nullptr;
    }

    // Initialize all level wave arrays to zero
    for (int i = 0; i < MAX_LEVEL; i++) {
        levels[i].numWaves = 0;
        for (int j = 0; j < MAX_WAVES; j++) {
            levels[i].numCustomSpawns[j] = 0;
        }
        for (int j = 0; j < MAX_WAVES; j++) {
            levels[i].waves[j] = {};
            levels[i].waves[j].spawnRate = 15;
            levels[i].waves[j].basicRate = -1; levels[i].waves[j].fastRate = -1;
            levels[i].waves[j].tankRate = -1; levels[i].waves[j].eliteRate = -1;
            levels[i].waves[j].launcherRate = -1; levels[i].waves[j].beamRate = -1;
            levels[i].waves[j].anchoredEliteRate = -1; levels[i].waves[j].anchoredEliteBeamRate = -1;
            levels[i].waves[j].rocketBossRate = -1; levels[i].waves[j].miniBossRate = -1;
        }
    }

    // Load levels from config file
    loadLevels("levels.cfg");

    bossActive = false;
    bossSwarmSpawned = false;
    bossSwarmRemaining = 0;
    playerDeathTimer = 0;
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

void Game::loadLevels(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Warning: Could not open %s, using empty levels\n", filename);
        return;
    }

    char line[256];
    int currentLevelIdx = -1;

    while (fgets(line, sizeof(line), file)) {
        // Strip newline
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        // Skip empty lines and comments
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '#') continue;

        int levelNum;
        int delayVal;

        if (sscanf(p, "level %d", &levelNum) == 1) {
            currentLevelIdx = levelNum - 1;
            if (currentLevelIdx < 0 || currentLevelIdx >= MAX_LEVEL) {
                printf("Warning: level %d out of range (1-%d), skipping\n", levelNum, MAX_LEVEL);
                currentLevelIdx = -1;
            }
        } else if (sscanf(p, "delay %d", &delayVal) == 1) {
            // Set delay on the NEXT wave to be added
            if (currentLevelIdx >= 0) {
                int w = levels[currentLevelIdx].numWaves;
                if (w < MAX_WAVES) {
                    // Store delay for the next wave (will be set when wave line is parsed)
                    levels[currentLevelIdx].waves[w].delayBefore = delayVal;
                }
            }
        } else if (strncmp(p, "spawn ", 6) == 0) {
            // Custom spawn line: spawn <type> <x> <delay_ticks>
            if (currentLevelIdx >= 0 && levels[currentLevelIdx].numWaves > 0) {
                int w = levels[currentLevelIdx].numWaves - 1;
                if (levels[currentLevelIdx].waves[w].isCustom &&
                    levels[currentLevelIdx].numCustomSpawns[w] < MAX_CUSTOM_SPAWNS) {
                    char typeName[32];
                    int sx, sdelay;
                    if (sscanf(p, "spawn %31s %d %d", typeName, &sx, &sdelay) == 3) {
                        int idx = levels[currentLevelIdx].numCustomSpawns[w];
                        CustomSpawn& cs = levels[currentLevelIdx].customSpawns[w][idx];
                        cs.x = sx;
                        cs.delayTicks = sdelay;
                        cs.spawned = false;
                        cs.beamMode = false;
                        cs.eliteBehavior = 0;
                        cs.tankBehavior = 0;

                        if (strcmp(typeName, "basic") == 0) cs.type = BASIC;
                        else if (strcmp(typeName, "fast") == 0) cs.type = FAST;
                        else if (strcmp(typeName, "tank") == 0) cs.type = TANK;
                        else if (strcmp(typeName, "tank_guard") == 0) { cs.type = TANK; cs.tankBehavior = 1; }
                        else if (strcmp(typeName, "tank_patrol") == 0) { cs.type = TANK; cs.tankBehavior = 2; }
                        else if (strcmp(typeName, "tank_carousel") == 0) { cs.type = TANK; cs.tankBehavior = 3; }
                        else if (strcmp(typeName, "elite") == 0) cs.type = ELITE;
                        else if (strcmp(typeName, "elite_straight") == 0) { cs.type = ELITE; cs.eliteBehavior = 1; }
                        else if (strcmp(typeName, "elite_dodging") == 0) { cs.type = ELITE; cs.eliteBehavior = 2; }
                        else if (strcmp(typeName, "launcher") == 0) cs.type = LAUNCHER;
                        else if (strcmp(typeName, "beam") == 0) cs.type = BEAM;
                        else if (strcmp(typeName, "anchorelite") == 0) cs.type = ANCHORED_ELITE;
                        else if (strcmp(typeName, "anchorelitebeam") == 0) { cs.type = ANCHORED_ELITE; cs.beamMode = true; }
                        else if (strcmp(typeName, "rocketboss") == 0) cs.type = ROCKET_BOSS;
                        else if (strcmp(typeName, "miniboss") == 0) cs.type = MINI_BOSS;
                        else { continue; }

                        levels[currentLevelIdx].numCustomSpawns[w]++;
                    }
                }
            }
        } else if (strncmp(p, "wave", 4) == 0) {
            if (currentLevelIdx >= 0 && levels[currentLevelIdx].numWaves < MAX_WAVES) {
                int w = levels[currentLevelIdx].numWaves;
                Wave& wave = levels[currentLevelIdx].waves[w];
                int savedDelay = wave.delayBefore;
                // 10 counts, isCustom, spawnRate, delayBefore, 10 rates = 23 fields
                wave = {};  // Zero everything
                wave.spawnRate = 15;
                wave.delayBefore = savedDelay;
                wave.basicRate = -1; wave.fastRate = -1; wave.tankRate = -1;
                wave.eliteRate = -1; wave.launcherRate = -1; wave.beamRate = -1;
                wave.anchoredEliteRate = -1; wave.anchoredEliteBeamRate = -1;
                wave.rocketBossRate = -1; wave.miniBossRate = -1;

                // Check for "wave custom"
                char afterWave[32] = "";
                sscanf(p + 4, " %31s", afterWave);

                if (strcmp(afterWave, "custom") == 0) {
                    wave.isCustom = true;
                    int inlineDelay;
                    if (sscanf(p, "wave custom delay %d", &inlineDelay) == 1) {
                        wave.delayBefore = inlineDelay;
                    }
                } else {
                    // Named format: wave basic 20:8 fast 10:3
                    char* tok = p + 4;
                    char key[32];
                    char valStr[32];
                    while (sscanf(tok, " %31s %31s", key, valStr) == 2) {
                        int val = 0, typeRate = -1;
                        char* colon = strchr(valStr, ':');
                        if (colon) {
                            *colon = '\0';
                            val = atoi(valStr);
                            typeRate = atoi(colon + 1);
                        } else {
                            val = atoi(valStr);
                        }

                        if (strcmp(key, "basic") == 0) { wave.basicCount = val; if (typeRate >= 0) wave.basicRate = typeRate; }
                        else if (strcmp(key, "fast") == 0) { wave.fastCount = val; if (typeRate >= 0) wave.fastRate = typeRate; }
                        else if (strcmp(key, "tank") == 0) { wave.tankCount = val; if (typeRate >= 0) wave.tankRate = typeRate; }
                        else if (strcmp(key, "elite") == 0) { wave.eliteCount = val; if (typeRate >= 0) wave.eliteRate = typeRate; }
                        else if (strcmp(key, "launcher") == 0) { wave.launcherCount = val; if (typeRate >= 0) wave.launcherRate = typeRate; }
                        else if (strcmp(key, "beam") == 0) { wave.beamCount = val; if (typeRate >= 0) wave.beamRate = typeRate; }
                        else if (strcmp(key, "anchorelite") == 0) { wave.anchoredEliteCount = val; if (typeRate >= 0) wave.anchoredEliteRate = typeRate; }
                        else if (strcmp(key, "anchorelitebeam") == 0) { wave.anchoredEliteBeamCount = val; if (typeRate >= 0) wave.anchoredEliteBeamRate = typeRate; }
                        else if (strcmp(key, "rocketboss") == 0) { wave.rocketBossCount = val; if (typeRate >= 0) wave.rocketBossRate = typeRate; }
                        else if (strcmp(key, "miniboss") == 0) { wave.miniBossCount = val; if (typeRate >= 0) wave.miniBossRate = typeRate; }
                        else if (strcmp(key, "spawnrate") == 0) wave.spawnRate = val;
                        else if (strcmp(key, "delay") == 0) wave.delayBefore = val;

                        while (*tok == ' ' || *tok == '\t') tok++;
                        while (*tok && *tok != ' ' && *tok != '\t') tok++;
                        while (*tok == ' ' || *tok == '\t') tok++;
                        while (*tok && *tok != ' ' && *tok != '\t') tok++;
                    }
                }
                levels[currentLevelIdx].numWaves++;
            }
        }
    }

    fclose(file);
    printf("Loaded %d levels from %s\n", currentLevelIdx + 1, filename);
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
    lastSpawnBasic = 0;
    lastSpawnFast = 0;
    lastSpawnTank = 0;
    lastSpawnElite = 0;
    lastSpawnLauncher = 0;
    lastSpawnBeam = 0;
    lastSpawnAnchoredElite = 0;
    lastSpawnAnchoredEliteBeam = 0;
    lastSpawnRocketBoss = 0;
    lastSpawnMiniBoss = 0;
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
    waveSpawnedLauncher = 0;
    waveSpawnedBeam = 0;
    waveSpawnedAnchoredElite = 0;
    waveSpawnedAnchoredEliteBeam = 0;
    waveSpawnedRocketBoss = 0;
    waveSpawnedMiniBoss = 0;
    allWavesComplete = false;
    waveDelayTimer = 0;
    waveTick = 0;
    bossActive = false;
    bossSwarmSpawned = false;
    bossSwarmRemaining = 0;
    playerDeathTimer = 0;

    // Reset custom spawn flags for all levels
    for (int l = 0; l < MAX_LEVEL; l++) {
        for (int w = 0; w < levels[l].numWaves; w++) {
            for (int s = 0; s < levels[l].numCustomSpawns[w]; s++) {
                levels[l].customSpawns[w][s].spawned = false;
            }
        }
    }
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
        // Always draw the screen text
        if (gameState == VICTORY) drawVictoryScreen(matrix);
        else drawGameOverScreen(matrix);

        if (levelDisplayTimer > 0) {
            levelDisplayTimer--;
            return;  // Brief input lockout
        }
        // Require a fresh press of any button (must release first, then press)
        bool anyPressed = input.button1 || input.button2 || input.button3;
        bool anyWasPressed = lastButton1;  // reuse as "any button was held"
        lastButton1 = anyPressed;
        if (anyPressed && !anyWasPressed) {
            setup();
            gameState = MAIN_MENU;
            lastButton1 = true;
            joystickDebounceTimer = 30;  // Prevent instant menu selection
        }
        return;
    } else if (gameState == PLAYER_DYING) {
        // Ship explosion sequence
        playerDeathTimer++;
        int px = player.get_x();
        int py = player.get_y();

        // Clear screen
        for (int ey = 0; ey < 64; ey++)
            for (int ex = 0; ex < 64; ex++)
                matrix->SetPixel(ey, ex, 18, 10, 14);

        // Spawn explosions around the ship position
        if (playerDeathTimer < 40 && playerDeathTimer % 3 == 0) {
            int ex = px + (rand() % 10) - 5;
            int ey = py + (rand() % 10) - 5;
            addExplosion(ex, ey);
        }

        // Draw explosions
        updateExplosions();
        drawExplosions(matrix);

        // Draw disintegrating ship for first part
        if (playerDeathTimer < 30) {
            int dropout = playerDeathTimer * 3;  // Increasing pixel loss
            auto sp = [matrix, dropout](int row, int col, int r, int g, int b) {
                if (col >= 0 && col < 64 && row >= 0 && row < 64) {
                    if ((rand() % 100) < dropout) return;
                    matrix->SetPixel(row, col, r, g, b);
                }
            };
            // Rough ship shape with color shifting to orange
            int shift = playerDeathTimer * 6;
            for (int dy = -2; dy <= 2; dy++) {
                int hw = 2 - abs(dy);
                for (int dx = -hw; dx <= hw; dx++) {
                    int r = 100 + shift > 255 ? 255 : 100 + shift;
                    int g = 150 > shift ? 150 - shift : 0;
                    int b = 200 > shift ? 200 - shift : 0;
                    sp(py + dy, px + dx, r, g, b);
                }
            }
        }

        drawHUD(matrix, 50);

        // After explosions finish, go straight to game over text
        if (playerDeathTimer > 60) {
            gameState = GAME_OVER;
            levelDisplayTimer = 5;  // Minimal input lockout
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

    // Check if player is dead — start death explosion
    if (player.get_health() <= 0 && gameState != PLAYER_DYING) {
        gameState = PLAYER_DYING;
        playerDeathTimer = 0;
        // Spawn initial explosion at player position
        addExplosion(player.get_x(), player.get_y());
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
                    enemies[i] = new BossAlien(32, -12);
                    bossActive = true;
                    break;
                }
            }
        }

        // Phase 3 swarm spawning - spawn fast aliens gradually
        if (bossSwarmRemaining > 0 && clock - lastSpawnTime > 8) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i] == nullptr) {
                    int rx = 5 + (rand() % 54);
                    enemies[i] = new FastAlien(rx, -5);
                    bossSwarmRemaining--;
                    lastSpawnTime = clock;
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

    // Wait for wave delay if active
    if (waveDelayTimer > 0) {
        waveDelayTimer--;
        return;
    }

    // Handle custom waves
    if (wave.isCustom) {
        waveTick++;
        int numSpawns = level.numCustomSpawns[currentWave];
        bool allSpawned = true;

        for (int s = 0; s < numSpawns; s++) {
            CustomSpawn& cs = level.customSpawns[currentWave][s];
            if (cs.spawned) continue;
            allSpawned = false;

            if (waveTick >= cs.delayTicks) {
                // Find empty slot and spawn
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i] != nullptr) continue;

                    int spawnY = -8;
                    switch (cs.type) {
                        case BASIC: enemies[i] = new BasicAlien(cs.x, spawnY); break;
                        case FAST: enemies[i] = new FastAlien(cs.x, spawnY); break;
                        case TANK: {
                            TankAlien* ta = new TankAlien(cs.x, spawnY);
                            ta->setBehavior(cs.tankBehavior);
                            enemies[i] = ta;
                            break;
                        }
                        case ELITE: {
                            EliteAlien* ea = new EliteAlien(cs.x, spawnY);
                            ea->setBehavior(cs.eliteBehavior);
                            enemies[i] = ea;
                            break;
                        }
                        case LAUNCHER: enemies[i] = new LauncherAlien(cs.x, spawnY); break;
                        case BEAM: enemies[i] = new BeamAlien(cs.x, spawnY); break;
                        case ANCHORED_ELITE: {
                            AnchoredElite* ae = new AnchoredElite(cs.x, spawnY);
                            if (cs.beamMode) ae->setBeamMode(true);
                            enemies[i] = ae;
                            break;
                        }
                        case ROCKET_BOSS: enemies[i] = new RocketBoss(cs.x, spawnY); break;
                        case MINI_BOSS: enemies[i] = new MiniBossAlien(cs.x, spawnY); break;
                        default: break;
                    }
                    cs.spawned = true;
                    break;
                }
            }
        }

        // Check if all custom spawns are done AND no enemies remain
        if (allSpawned) {
            bool anyAlive = false;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i] != nullptr) { anyAlive = true; break; }
            }
            if (!anyAlive) {
                currentWave++;
                waveTick = 0;
                if (currentWave >= level.numWaves) {
                    allWavesComplete = true;
                } else {
                    waveDelayTimer = level.waves[currentWave].delayBefore;
                }
            }
        }
        return;
    }

    // Check if current wave is fully spawned
    if (waveSpawnedBasic >= wave.basicCount &&
        waveSpawnedFast >= wave.fastCount &&
        waveSpawnedTank >= wave.tankCount &&
        waveSpawnedElite >= wave.eliteCount &&
        waveSpawnedLauncher >= wave.launcherCount &&
        waveSpawnedBeam >= wave.beamCount &&
        waveSpawnedAnchoredElite >= wave.anchoredEliteCount &&
        waveSpawnedAnchoredEliteBeam >= wave.anchoredEliteBeamCount &&
        waveSpawnedRocketBoss >= wave.rocketBossCount &&
        waveSpawnedMiniBoss >= wave.miniBossCount) {
        // Wait for all enemies to clear before next wave
        bool anyAlive = false;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i] != nullptr) { anyAlive = true; break; }
        }
        if (anyAlive) return;  // Wait for screen to clear

        // Advance to next wave
        currentWave++;
        waveSpawnedBasic = 0;
        waveSpawnedFast = 0;
        waveSpawnedTank = 0;
        waveSpawnedElite = 0;
        waveSpawnedLauncher = 0;
        waveSpawnedBeam = 0;
        waveSpawnedMiniBoss = 0;
        if (currentWave >= level.numWaves) {
            allWavesComplete = true;
        } else {
            // Apply delay before next wave if specified
            waveDelayTimer = level.waves[currentWave].delayBefore;
        }
        return;
    }

    // Helper: get the effective spawn rate for a type (-1 means use default)
    auto getRate = [&](int typeRate) -> int {
        return (typeRate >= 0) ? typeRate : wave.spawnRate;
    };

    // Try to spawn each type independently on its own timer
    struct SpawnEntry {
        int remaining;      // How many left to spawn
        int rate;           // Spawn rate for this type
        int* lastTime;      // Per-type last spawn timer
        int* spawnedCount;  // Counter to increment
        AlienType type;
        bool beamMode;      // For anchored elite beam variant
    };

    SpawnEntry entries[] = {
        {wave.basicCount - waveSpawnedBasic, getRate(wave.basicRate), &lastSpawnBasic, &waveSpawnedBasic, BASIC, false},
        {wave.fastCount - waveSpawnedFast, getRate(wave.fastRate), &lastSpawnFast, &waveSpawnedFast, FAST, false},
        {wave.tankCount - waveSpawnedTank, getRate(wave.tankRate), &lastSpawnTank, &waveSpawnedTank, TANK, false},
        {wave.eliteCount - waveSpawnedElite, getRate(wave.eliteRate), &lastSpawnElite, &waveSpawnedElite, ELITE, false},
        {wave.launcherCount - waveSpawnedLauncher, getRate(wave.launcherRate), &lastSpawnLauncher, &waveSpawnedLauncher, LAUNCHER, false},
        {wave.beamCount - waveSpawnedBeam, getRate(wave.beamRate), &lastSpawnBeam, &waveSpawnedBeam, BEAM, false},
        {wave.anchoredEliteCount - waveSpawnedAnchoredElite, getRate(wave.anchoredEliteRate), &lastSpawnAnchoredElite, &waveSpawnedAnchoredElite, ANCHORED_ELITE, false},
        {wave.anchoredEliteBeamCount - waveSpawnedAnchoredEliteBeam, getRate(wave.anchoredEliteBeamRate), &lastSpawnAnchoredEliteBeam, &waveSpawnedAnchoredEliteBeam, ANCHORED_ELITE, true},
        {wave.rocketBossCount - waveSpawnedRocketBoss, getRate(wave.rocketBossRate), &lastSpawnRocketBoss, &waveSpawnedRocketBoss, ROCKET_BOSS, false},
        {wave.miniBossCount - waveSpawnedMiniBoss, getRate(wave.miniBossRate), &lastSpawnMiniBoss, &waveSpawnedMiniBoss, MINI_BOSS, false},
    };

    for (auto& e : entries) {
        if (e.remaining <= 0) continue;
        if (clock - *e.lastTime <= e.rate) continue;

        // Find an empty slot
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i] != nullptr) continue;

            int randomX = 10 + (rand() % 44);
            int spawnY = -8;

            // For tanks, ensure no overlap with existing tanks
            if (e.type == TANK) {
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

            // Create the alien
            switch(e.type) {
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
                case LAUNCHER:
                    enemies[i] = new LauncherAlien(randomX, spawnY);
                    break;
                case BEAM:
                    enemies[i] = new BeamAlien(randomX, spawnY);
                    break;
                case ANCHORED_ELITE: {
                    AnchoredElite* ae = new AnchoredElite(randomX, spawnY);
                    if (e.beamMode) ae->setBeamMode(true);
                    enemies[i] = ae;
                    break;
                }
                case ROCKET_BOSS:
                    enemies[i] = new RocketBoss(randomX, spawnY);
                    break;
                case MINI_BOSS: {
                    int mbPositions[] = {16, 32, 48};
                    int mbX = mbPositions[*e.spawnedCount % 3];
                    enemies[i] = new MiniBossAlien(mbX, spawnY - *e.spawnedCount * 3);
                    break;
                }
                default: break;
            }

            (*e.spawnedCount)++;
            *e.lastTime = clock;
            break;
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
            anyEnemiesAlive = true;  // Keep boss "alive" during death sequence
            BossAlien* dyingBoss = dynamic_cast<BossAlien*>(enemies[i]);
            if (dyingBoss && dyingBoss->wantsExplosion()) {
                int ex = enemies[i]->get_x() + (rand() % 24) - 12;
                int ey = enemies[i]->get_y() + (rand() % 16) - 8;
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
            } else if (enemies[i]->getType() == LAUNCHER) {
                moveFrequency = 3;  // LAUNCHER moves every 3 frames
            } else if (enemies[i]->getType() == BEAM) {
                moveFrequency = 4;  // BEAM moves slowly
            } else if (enemies[i]->getType() == ANCHORED_ELITE) {
                moveFrequency = 3;
            } else if (enemies[i]->getType() == ROCKET_BOSS) {
                moveFrequency = 2;
            } else if (enemies[i]->getType() == MINI_BOSS) {
                moveFrequency = 3;
            } else if (enemies[i]->getType() == BOSS) {
                BossAlien* boss = dynamic_cast<BossAlien*>(enemies[i]);
                if (boss) {
                    if (boss->getPhase() == 1) moveFrequency = 3;
                    else if (boss->getPhase() == 2) moveFrequency = 2;
                    else if (boss->getPhase() == 3) moveFrequency = 3;  // Slow during swarm
                    else moveFrequency = 1;  // Phase 4 - fast
                }
            }

            if (clock % moveFrequency == 0) {
                enemies[i]->move(0, 1);
            }

            // Launcher and beam stay on screen — anchored, never scroll off
            // Guard tanks stay on screen
            if (enemies[i]->getType() == TANK) {
                TankAlien* ta = dynamic_cast<TankAlien*>(enemies[i]);
                // Guard tanks stay on screen, carousel tanks only stay until they scroll off bottom
                if (ta && ta->getBehavior() == 1) continue;
            }
            if (enemies[i]->getType() == LAUNCHER || enemies[i]->getType() == BEAM || enemies[i]->getType() == ANCHORED_ELITE || enemies[i]->getType() == ROCKET_BOSS) {
                continue;
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
            int bottomMargin = (enemies[i]->getType() == TANK) ? 4 : (enemies[i]->getType() == MINI_BOSS) ? 5 : 3;
            if (enemies[i]->get_y() > 63 + bottomMargin) {
                delete enemies[i];
                enemies[i] = nullptr;
            }
        } else {
            // Health is 0, still in death animation
            anyEnemiesAlive = true;
        }
    }

    // Boss fight logic
    if (currentLevel == 6) {
        // Find the boss
        BossAlien* boss = nullptr;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i] != nullptr && enemies[i]->getType() == BOSS && enemies[i]->get_health() > 0) {
                boss = dynamic_cast<BossAlien*>(enemies[i]);
                break;
            }
        }

        // Phase 3: trigger swarm spawn
        if (boss && boss->getPhase() == 3 && !bossSwarmSpawned) {
            bossSwarmSpawned = true;
            bossSwarmRemaining = 40;
        }

        // Phase 3 -> 4: check if swarm is cleared
        if (boss && boss->getPhase() == 3 && bossSwarmSpawned && bossSwarmRemaining == 0) {
            // Count non-boss enemies still alive
            int swarmAlive = 0;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i] != nullptr && enemies[i]->getType() != BOSS) swarmAlive++;
            }
            if (swarmAlive == 0) {
                boss->setPhase(4);
                bossSwarmSpawned = false;
            }
        }

        // Check if boss is dead
        if (bossActive && !boss) {
            bossActive = false;
            bossSwarmSpawned = false;
            bossSwarmRemaining = 0;
    playerDeathTimer = 0;
            allWavesComplete = true;
        }

        // Once boss is gone, wait for all enemies AND explosions to clear
        if (!bossActive && allWavesComplete && !anyEnemiesAlive) {
            // Check no active explosions remain
            bool explosionsActive = false;
            for (int i = 0; i < MAX_EXPLOSIONS; i++) {
                if (explosions[i].active) { explosionsActive = true; break; }
            }
            if (!explosionsActive) {
                gameState = VICTORY;
                levelDisplayTimer = 100;  // Idle on empty screen then show text
            }
        }
        return;
    }

    // Despawn guard tanks if no anchored aliens (launcher/beam/anchored_elite) remain
    bool anyAnchored = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr && enemies[i]->get_health() > 0) {
            AlienType t = enemies[i]->getType();
            if (t == LAUNCHER || t == BEAM || t == ANCHORED_ELITE) {
                anyAnchored = true;
                break;
            }
        }
    }
    if (!anyAnchored) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i] != nullptr && enemies[i]->getType() == TANK) {
                TankAlien* ta = dynamic_cast<TankAlien*>(enemies[i]);
                if (ta && ta->getBehavior() == 1) {
                    enemies[i]->takeDamage(1000);  // Destroy guard tanks
                }
            }
        }
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
            AlienType ht = enemies[j]->getType();
            int hitRangeX = (ht == BOSS) ? 13 : (ht == MINI_BOSS) ? 5 : 2;
            int hitRangeY = (ht == BOSS) ? 10 : (ht == MINI_BOSS) ? 4 : 2;

            if (dx >= -hitRangeX && dx <= hitRangeX && dy >= -hitRangeY && dy <= hitRangeY) {
                // Boss weak point check: after shield is down, only weak points take damage
                if (enemies[j]->getType() == BOSS) {
                    BossAlien* boss = dynamic_cast<BossAlien*>(enemies[j]);
                    if (boss && boss->getShield() <= 0) {
                        // Check if bullet hits a weak point (3 points with radius 3)
                        // Left: (ex-9, ey+5), Center: (ex, ey+6), Right: (ex+9, ey+5)
                        bool hitWeak = false;
                        int wpRadius = 3;
                        if (abs(bx - (ex - 9)) <= wpRadius && abs(by - (ey + 5)) <= wpRadius) hitWeak = true;
                        if (abs(bx - ex) <= wpRadius && abs(by - (ey + 6)) <= wpRadius) hitWeak = true;
                        if (abs(bx - (ex + 9)) <= wpRadius && abs(by - (ey + 5)) <= wpRadius) hitWeak = true;
                        if (!hitWeak) {
                            break;  // Missed weak point, bullet passes through
                        }
                    }
                }
                // Weak point hits deal double damage
                bool isBossWeak = false;
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
            int laserHitRange = (enemies[j]->getType() == BOSS) ? 10 : (enemies[j]->getType() == MINI_BOSS) ? 4 : 1;

            if (dx >= -laserHitRange && dx <= laserHitRange && ey < laserY && ey >= 0) {
                // Boss weak point check for laser
                if (enemies[j]->getType() == BOSS) {
                    BossAlien* boss = dynamic_cast<BossAlien*>(enemies[j]);
                    if (boss && boss->getShield() <= 0) {
                        // Laser must hit near a weak point x-position
                        bool hitWeak = false;
                        if (abs(laserX - (ex - 9)) <= 3) hitWeak = true;
                        if (abs(laserX - ex) <= 3) hitWeak = true;
                        if (abs(laserX - (ex + 9)) <= 3) hitWeak = true;
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
        int shipHitRange = (enemies[j]->getType() == BOSS) ? 13 : (enemies[j]->getType() == MINI_BOSS) ? 5 : 3;

        if (dx >= -shipHitRange && dx <= shipHitRange && dy >= -shipHitRange && dy <= shipHitRange) {
            if (enemies[j]->getType() == BOSS) {
                player.takeDamage(50);
                enemies[j]->takeDamage(20);  // Boss takes some damage from collision
            } else if (enemies[j]->getType() == ELITE) {
                EliteAlien* ea = dynamic_cast<EliteAlien*>(enemies[j]);
                if (ea && ea->isDashing()) {
                    player.takeDamage(60);  // Dashing elite hits harder
                } else {
                    player.takeDamage(30);
                }
                enemies[j]->takeDamage(1000);
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
    waveSpawnedLauncher = 0;
    waveSpawnedBeam = 0;
    waveSpawnedAnchoredElite = 0;
    waveSpawnedAnchoredEliteBeam = 0;
    waveSpawnedRocketBoss = 0;
    waveSpawnedMiniBoss = 0;
    allWavesComplete = false;
    waveDelayTimer = 0;
    waveTick = 0;
    bossActive = false;
    bossSwarmSpawned = false;
    bossSwarmRemaining = 0;
    playerDeathTimer = 0;

    // Reset custom spawn flags for the new level
    if (currentLevel >= 1 && currentLevel <= MAX_LEVEL) {
        LevelConfig& lvl = levels[currentLevel - 1];
        for (int w = 0; w < lvl.numWaves; w++) {
            for (int s = 0; s < lvl.numCustomSpawns[w]; s++) {
                lvl.customSpawns[w][s].spawned = false;
            }
        }
    }

    levelDisplayTimer = 100;  // Show new level for 100 ticks
    levelStartDelay = 150;  // 1.5 second delay before starting
    levelActive = false;  // Stop spawning during delay

    // Immediately delete all enemies when advancing level
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr) {
            delete enemies[i];
            enemies[i] = nullptr;
        }
    }

    // Clear all enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemyBullets[i].deactivate();
    }
}

AlienType Game::selectWaveAlienType() {
    if (currentLevel < 1 || currentLevel > MAX_LEVEL) return BASIC;
    LevelConfig& level = levels[currentLevel - 1];
    if (currentWave >= level.numWaves) return BASIC;

    Wave& wave = level.waves[currentWave];

    // Build list of types that still need spawning
    AlienType available[7];
    int count = 0;
    if (waveSpawnedBasic < wave.basicCount) available[count++] = BASIC;
    if (waveSpawnedFast < wave.fastCount) available[count++] = FAST;
    if (waveSpawnedTank < wave.tankCount) available[count++] = TANK;
    if (waveSpawnedElite < wave.eliteCount) available[count++] = ELITE;
    if (waveSpawnedLauncher < wave.launcherCount) available[count++] = LAUNCHER;
    if (waveSpawnedBeam < wave.beamCount) available[count++] = BEAM;
    if (waveSpawnedMiniBoss < wave.miniBossCount) available[count++] = MINI_BOSS;

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
    // Any of the 3 buttons can select (edge-detected via lastButton1 tracking any press)
    bool anyNow = input.button1 || input.button2 || input.button3;
    bool anyPrev = lastButton1;
    lastButton1 = anyNow;
    bool buttonPressed = anyNow && !anyPrev;
    
    // Decrement debounce timer
    if (joystickDebounceTimer > 0) {
        joystickDebounceTimer--;
    }
    
    if (gameState == MAIN_MENU) {
        // Navigate menu with joystick Y (with debouncing)
        if (joystickDebounceTimer == 0) {
            if (input.joystick_y < -50) {  // Up
                menuSelection = (menuSelection - 1 + 3) % 3;
                joystickDebounceTimer = 10;  // Prevent rapid changes
            } else if (input.joystick_y > 50) {  // Down
                menuSelection = (menuSelection + 1) % 3;
                joystickDebounceTimer = 10;  // Prevent rapid changes
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
                joystickDebounceTimer = 12;  // Add debounce to prevent immediate selection
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
                joystickDebounceTimer = 10;  // Prevent rapid changes
            } else if (input.joystick_y > 50) {  // Down
                levelSelectChoice = (levelSelectChoice + 1);
                if (levelSelectChoice > MAX_LEVEL + 1) levelSelectChoice = MAX_LEVEL + 1;  // +1 for BACK
                joystickDebounceTimer = 10;  // Prevent rapid changes
            }
        }
        
        // Select with button1, back with button3 (only if not in debounce)
        if (buttonPressed && joystickDebounceTimer == 0) {
            if (levelSelectChoice == MAX_LEVEL + 1) {
                // BACK selected
                gameState = MAIN_MENU;
            } else {
                int selectedLevel = levelSelectChoice;
                setup();
                currentLevel = selectedLevel;
                levelSelectChoice = selectedLevel;
                levelDisplayTimer = 100;
                levelStartDelay = 150;
                levelActive = false;
                gameState = PLAYING;
            }
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
        } else if (c == 'B') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            safeSetPixel(y, charX + 1, r, g, b);
            safeSetPixel(y, charX, r, g, b);
            safeSetPixel(y + 1, charX, r, g, b);
            safeSetPixel(y + 2, charX + 1, r, g, b);
            safeSetPixel(y + 2, charX, r, g, b);
            safeSetPixel(y + 3, charX, r, g, b);
            safeSetPixel(y + 4, charX + 1, r, g, b);
            safeSetPixel(y + 4, charX, r, g, b);
        } else if (c == 'K') {
            for (int j = 0; j < 5; j++) safeSetPixel(y + j, charX + 2, r, g, b);
            safeSetPixel(y, charX, r, g, b);
            safeSetPixel(y + 1, charX + 1, r, g, b);
            safeSetPixel(y + 2, charX + 1, r, g, b);
            safeSetPixel(y + 3, charX + 1, r, g, b);
            safeSetPixel(y + 4, charX, r, g, b);
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
    
    // Draw levels on separate rows (0 = free fly, 1-6 = combat levels, -1 = back)
    int startX = 15;
    for (int i = 0; i <= MAX_LEVEL; i++) {
        char levelStr[2] = {(char)('0' + i), '\0'};
        int rowY = 15 + i * 7;
        if (i == levelSelectChoice) {
            drawText(matrix, levelStr, startX, rowY, 255, 255, 0);
        } else {
            drawText(matrix, levelStr, startX, rowY, 100, 100, 100);
        }
    }
    // BACK option at bottom-right
    if (levelSelectChoice == MAX_LEVEL + 1) {
        drawText(matrix, "BACK", 44, 57, 255, 255, 0);
    } else {
        drawText(matrix, "BACK", 44, 57, 100, 100, 100);
    }
}

void Game::updateEnemyBullets() {
    int px = player.get_x();
    int py = player.get_y();
    static int homingFrame = 0;
    homingFrame++;

    // Update all enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].isActive()) {
            // Homing missiles — steer toward player, move every 2nd frame
            if (enemyBullets[i].getHoming()) {
                int bx = enemyBullets[i].getX();
                // Steer toward player
                if (bx < px - 2) enemyBullets[i].setDx(1);
                else if (bx > px + 2) enemyBullets[i].setDx(-1);
                else enemyBullets[i].setDx(0);

                // Use bullet's own Y position (odd/even) to throttle
                // Both missiles use the same global frame counter
                if (homingFrame % 2 == 0) {
                    enemyBullets[i].setDy(1);
                } else {
                    enemyBullets[i].setDy(0);
                    enemyBullets[i].setDx(0);
                }
            }

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
                for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                    if (!enemyBullets[j].isActive()) {
                        enemyBullets[j] = Bullet(elite->get_x(), elite->get_y() + 3, 0, 3, 140, 50, 160);
                        elite->resetShotCooldown();
                        break;
                    }
                }
            }
        }
        // Make anchored elites shoot or fire beams
        if (enemies[i] != nullptr && enemies[i]->getType() == ANCHORED_ELITE && enemies[i]->get_health() > 0) {
            AnchoredElite* ae = dynamic_cast<AnchoredElite*>(enemies[i]);
            if (ae) {
                if (ae->isBeamMode()) {
                    ae->tickBeam();
                    // Beam damage
                    if (ae->isFiring()) {
                        int bx = ae->getBeamX();
                        int by = ae->get_y() + 3;
                        if (px >= bx - 1 && px <= bx + 1 && py >= by) {
                            player.takeDamage(1);
                        }
                    }
                } else if (ae->shouldShoot()) {
                    for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                        if (!enemyBullets[j].isActive()) {
                            enemyBullets[j] = Bullet(ae->get_x(), ae->get_y() + 3, 0, 3, 140, 50, 160);
                            ae->resetShotCooldown();
                            break;
                        }
                    }
                }
            }
        }
        // Make rocket boss fire homing rockets or scatter
        if (enemies[i] != nullptr && enemies[i]->getType() == ROCKET_BOSS && enemies[i]->get_health() > 0) {
            RocketBoss* rb = dynamic_cast<RocketBoss*>(enemies[i]);
            if (rb && rb->shouldShoot()) {
                int rbx = rb->get_x();
                int rby = rb->get_y();
                if (rb->getAttackMode() == 0) {
                    // Homing rockets
                    if (rb->getPhase() == 2) {
                        // Phase 2: fire from BOTH pods simultaneously
                        int sides[2] = {-3, 3};
                        for (int s = 0; s < 2; s++) {
                            for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                                if (!enemyBullets[j].isActive()) {
                                    int bulletDx = (px > rbx + sides[s]) ? 1 : (px < rbx + sides[s]) ? -1 : 0;
                                    enemyBullets[j] = Bullet(rbx + sides[s], rby + 4, bulletDx, 1, 240, 70, 160);
                                    enemyBullets[j].setMissile(true);
                                    enemyBullets[j].setHoming(true);
                                    break;
                                }
                            }
                        }
                    } else {
                        // Phase 1: fire from one pod at a time
                        int side = (rand() % 2 == 0) ? -3 : 3;
                        for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                            if (!enemyBullets[j].isActive()) {
                                int bulletDx = (px > rbx) ? 1 : (px < rbx) ? -1 : 0;
                                enemyBullets[j] = Bullet(rbx + side, rby + 4, bulletDx, 1, 240, 70, 160);
                                enemyBullets[j].setMissile(true);
                                enemyBullets[j].setHoming(true);
                                break;
                            }
                        }
                    }
                } else {
                    // Scatter — 12 bullets in a wide fan
                    int dirs[12][2] = {
                        {0, 3},    // Fast center
                        {0, 2},    // Slow center
                        {-1, 3},   // Slight left fast
                        {1, 3},    // Slight right fast
                        {-1, 2},   // Slight left
                        {1, 2},    // Slight right
                        {-2, 2},   // Mid left
                        {2, 2},    // Mid right
                        {-2, 1},   // Wide left slow
                        {2, 1},    // Wide right slow
                        {-3, 1},   // Extra wide left
                        {3, 1},    // Extra wide right
                    };
                    int spawned = 0;
                    for (int j = 0; j < MAX_ENEMY_BULLETS && spawned < 12; j++) {
                        if (!enemyBullets[j].isActive()) {
                            enemyBullets[j] = Bullet(rbx, rby + 6, dirs[spawned][0], dirs[spawned][1], 200, 50, 40);
                            enemyBullets[j].setScatter(true);
                            spawned++;
                        }
                    }
                }
                rb->resetShotCooldown();
            }
        }
        // Make launcher aliens fire slow blinking missiles
        if (enemies[i] != nullptr && enemies[i]->getType() == LAUNCHER && enemies[i]->get_health() > 0) {
            LauncherAlien* launcher = dynamic_cast<LauncherAlien*>(enemies[i]);
            if (launcher && launcher->shouldShoot()) {
                for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                    if (!enemyBullets[j].isActive()) {
                        // Aimed slow missile toward player
                        int lx = launcher->get_x();
                        int ly = launcher->get_y() + 3;
                        int bulletDx = (px > lx + 2) ? 1 : (px < lx - 2) ? -1 : 0;
                        enemyBullets[j] = Bullet(lx, ly, bulletDx, 1, 240, 70, 160);
                        enemyBullets[j].setMissile(true);
                        enemyBullets[j].setHoming(true);
                        launcher->resetShotCooldown();
                        break;
                    }
                }
            }
        }
        // Make beam aliens charge and fire
        if (enemies[i] != nullptr && enemies[i]->getType() == BEAM && enemies[i]->get_health() > 0) {
            BeamAlien* beam = dynamic_cast<BeamAlien*>(enemies[i]);
            if (beam) {
                beam->tickBeam();
                // Only start charging once cooldown has elapsed
                // (beamCooldown is decremented in move(), reaches 0 when ready)
                // Beam damage — check if player is in the beam column
                if (beam->isFiring()) {
                    int bx = beam->getBeamX();
                    int by = beam->get_y() + 3;
                    if (px >= bx - 1 && px <= bx + 1 && py >= by) {
                        player.takeDamage(1);  // Continuous damage each frame
                    }
                }
            }
        }
        // Make mini-boss shoot from turret
        if (enemies[i] != nullptr && enemies[i]->getType() == MINI_BOSS && enemies[i]->get_health() > 0) {
            MiniBossAlien* mb = dynamic_cast<MiniBossAlien*>(enemies[i]);
            if (mb) {
                mb->updateTurret(px);

                if (mb->shouldShoot()) {
                    // Count active mini-bosses
                    int mbCount = 0;
                    for (int k = 0; k < MAX_ENEMIES; k++) {
                        if (enemies[k] != nullptr && enemies[k]->getType() == MINI_BOSS && enemies[k]->get_health() > 0)
                            mbCount++;
                    }

                    int turretX = mb->get_x() + mb->getTurretX();
                    int fireY = mb->get_y() + 6;
                    // All modes fire aimed bullets, scaling fire rate by survivors
                    int bulletDx = (px > turretX) ? 1 : (px < turretX) ? -1 : 0;
                    for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                        if (!enemyBullets[j].isActive()) {
                            enemyBullets[j] = Bullet(turretX, fireY, bulletDx, 2, 200, 50, 80);
                            break;
                        }
                    }
                    // Solo: also fire a homing missile alongside the aimed bullet
                    if (mbCount == 1) {
                        for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                            if (!enemyBullets[j].isActive()) {
                                enemyBullets[j] = Bullet(turretX, fireY, 0, 1, 240, 70, 160);
                                enemyBullets[j].setMissile(true);
                                enemyBullets[j].setHoming(true);
                                break;
                            }
                        }
                    }
                    // Scale fire rate by survivors — faster as fewer remain
                    if (mbCount >= 3) mb->setShotCooldown(30 + (rand() % 15));
                    else if (mbCount == 2) mb->setShotCooldown(14 + (rand() % 8));
                    else mb->setShotCooldown(22 + (rand() % 12));  // Solo: fast aimed + homing
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
                    // Scatter shot - 6 bullets in a downward fan
                    int dirs[6][2] = {
                        {0, 2},    // Straight down
                        {-1, 2},   // Down-left
                        {1, 2},    // Down-right
                        {-2, 1},   // Wide left
                        {2, 1},    // Wide right
                        {0, 1}     // Slow center
                    };
                    int spawned = 0;
                    for (int j = 0; j < MAX_ENEMY_BULLETS && spawned < 6; j++) {
                        if (!enemyBullets[j].isActive()) {
                            enemyBullets[j] = Bullet(bx, by + 9, dirs[spawned][0], dirs[spawned][1], 180, 25, 25);
                            spawned++;
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
                    // Also fire 2 slow homing missiles from the pincers
                    int homingSpawned = 0;
                    for (int j = 0; j < MAX_ENEMY_BULLETS && homingSpawned < 2; j++) {
                        if (!enemyBullets[j].isActive()) {
                            int hx = (homingSpawned == 0) ? bx - 9 : bx + 9;
                            enemyBullets[j] = Bullet(hx, by + 7, 0, 1, 200, 40, 40);
                            enemyBullets[j].setHoming(true);
                            enemyBullets[j].setLarge(true);
                            homingSpawned++;
                        }
                    }
                } else {
                    // Phase 4: alternate scatter and beam
                    if (boss->getAttackPattern() == 0) {
                        // Powerful scatter - 10 bullets in wide fan
                        int dirs[10][2] = {
                            {0, 3},    // Straight down fast
                            {-1, 3},   // Down-left
                            {1, 3},    // Down-right
                            {-2, 2},   // Wide left
                            {2, 2},    // Wide right
                            {-3, 1},   // Very wide left
                            {3, 1},    // Very wide right
                            {0, 2},    // Slow center
                            {-1, 2},   // Slow left
                            {1, 2}     // Slow right
                        };
                        int spawned = 0;
                        for (int j = 0; j < MAX_ENEMY_BULLETS && spawned < 10; j++) {
                            if (!enemyBullets[j].isActive()) {
                                enemyBullets[j] = Bullet(bx, by + 9, dirs[spawned][0], dirs[spawned][1], 220, 40, 40);
                                spawned++;
                            }
                        }
                    }
                    // Beam attack is handled separately below (continuous)
                }
                boss->resetShotCooldown();
            }

            // Beam damage (rendering handled in drawEnemyBullets)
            if (boss->getBeamTimer() > 0) {
                int beamX = boss->get_x();
                int beamStartY = boss->get_y() + 9;
                int px = player.get_x();
                int py = player.get_y();
                if (abs(px - beamX) <= 2 && py > beamStartY) {
                    player.takeDamage(3);
                }
            }

            break;
        }
    }
}

void Game::drawEnemyBullets(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].isActive()) {
            enemyBullets[i].draw(matrix);
            // Launcher missiles: slow, blinking 2x2 with smoke trail
            if (enemyBullets[i].getMissile()) {
                int bx = enemyBullets[i].getX();
                int by = enemyBullets[i].getY();
                int tick = enemyBullets[i].getTick();
                // Blink the missile body between bright pink and dim
                bool bright = (tick / 4) % 2 == 0;
                int mr = bright ? 240 : 90;
                int mg = bright ? 70 : 25;
                int mb = bright ? 160 : 60;
                // Draw 2x1 missile body
                if (bx >= 0 && bx < 64 && by >= 0 && by < 64)
                    matrix->SetPixel(by, bx, mr, mg, mb);
                if (bx + 1 >= 0 && bx + 1 < 64 && by >= 0 && by < 64)
                    matrix->SetPixel(by, bx + 1, mr * 3 / 4, mg * 3 / 4, mb * 3 / 4);
                // Smoke trail behind - pinkish
                if (by - 1 >= 0 && by - 1 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 1, bx, 70, 25, 50);
                if (by - 2 >= 0 && by - 2 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 2, bx, 40, 15, 30);
            }
            // Homing missiles: 2px tall, 1px wide (trail pixel behind)
            if (enemyBullets[i].getHoming()) {
                int bx = enemyBullets[i].getX();
                int by = enemyBullets[i].getY();
                if (by - 1 >= 0 && by - 1 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 1, bx, 140, 35, 55);  // Dimmer trail pixel
            }
            // Only draw trail for fast straight bullets (elite/boss phase 3)
            if (enemyBullets[i].getDy() >= 3 && !enemyBullets[i].getHoming()) {
                int bx = enemyBullets[i].getX();
                int by = enemyBullets[i].getY();
                if (by - 1 >= 0 && by - 1 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 1, bx, 100, 35, 120);
                if (by - 2 >= 0 && by - 2 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 2, bx, 60, 20, 75);
            }
        }
    }

    // Draw boss beam charge + beam attack
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i] != nullptr && enemies[i]->getType() == BOSS && enemies[i]->get_health() > 0) {
            BossAlien* boss = dynamic_cast<BossAlien*>(enemies[i]);
            if (!boss) break;
            int beamX = boss->get_x();
            int gunY = boss->get_y() + 8;

            // Charging animation — growing energy ball at gun tip
            int charge = boss->getBeamChargeTimer();
            if (charge > 0) {
                int intensity = (20 - charge) * 12;  // 0 to 240
                if (intensity > 255) intensity = 255;
                int pulse = ((charge % 2) == 0) ? 40 : 0;
                // Flickering charge point
                if (gunY >= 0 && gunY < 64 && beamX >= 0 && beamX < 64)
                    matrix->SetPixel(gunY, beamX, intensity + pulse > 255 ? 255 : intensity + pulse, 30, 30);
                // Growing glow around charge point
                if (charge < 15) {
                    if (gunY + 1 < 64 && beamX >= 0 && beamX < 64)
                        matrix->SetPixel(gunY + 1, beamX, intensity / 2, 15, 20);
                    if (beamX - 1 >= 0 && gunY >= 0 && gunY < 64)
                        matrix->SetPixel(gunY, beamX - 1, intensity / 3, 10, 15);
                    if (beamX + 1 < 64 && gunY >= 0 && gunY < 64)
                        matrix->SetPixel(gunY, beamX + 1, intensity / 3, 10, 15);
                }
                // Warning line preview (faint dashed line)
                if (charge < 10) {
                    for (int row = gunY + 2; row < 64; row += 3) {
                        if (beamX >= 0 && beamX < 64 && row < 64)
                            matrix->SetPixel(row, beamX, intensity / 4, 8, 10);
                    }
                }
            }

            // Active beam
            if (boss->getBeamTimer() > 0) {
                for (int row = gunY; row < 64; row++) {
                    int flicker = (rand() % 3 == 0) ? 30 : 0;
                    if (beamX - 1 >= 0 && beamX - 1 < 64 && row >= 0 && row < 64)
                        matrix->SetPixel(row, beamX - 1, 150 + flicker, 20, 30 + flicker);
                    if (beamX >= 0 && beamX < 64 && row >= 0 && row < 64)
                        matrix->SetPixel(row, beamX, 255, 50 + flicker, 60 + flicker);
                    if (beamX + 1 >= 0 && beamX + 1 < 64 && row >= 0 && row < 64)
                        matrix->SetPixel(row, beamX + 1, 150 + flicker, 20, 30 + flicker);
                }
            }
            break;
        }
    }
}

void Game::eraseEnemyBullets(RGBMatrix *matrix) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].isActive()) {
            enemyBullets[i].erase(matrix);
            // Erase launcher missile trail and extra pixel
            if (enemyBullets[i].getMissile()) {
                int bx = enemyBullets[i].getX();
                int by = enemyBullets[i].getY();
                if (bx + 1 >= 0 && bx + 1 < 64 && by >= 0 && by < 64)
                    matrix->SetPixel(by, bx + 1, 18, 10, 14);
                if (by - 1 >= 0 && by - 1 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 1, bx, 18, 10, 14);
                if (by - 2 >= 0 && by - 2 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 2, bx, 18, 10, 14);
            }
            // Erase homing trail pixel
            if (enemyBullets[i].getHoming()) {
                int bx = enemyBullets[i].getX();
                int by = enemyBullets[i].getY();
                if (by - 1 >= 0 && by - 1 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 1, bx, 18, 10, 14);
            }
            // Erase trail only for fast straight bullets
            if (enemyBullets[i].getDy() >= 3 && !enemyBullets[i].getHoming()) {
                int bx = enemyBullets[i].getX();
                int by = enemyBullets[i].getY();
                if (by - 1 >= 0 && by - 1 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 1, bx, 18, 10, 14);
                if (by - 2 >= 0 && by - 2 < 64 && bx >= 0 && bx < 64)
                    matrix->SetPixel(by - 2, bx, 18, 10, 14);
            }
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