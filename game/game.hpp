#ifndef GAME_HPP
#define GAME_HPP
#include "ship.hpp"
#include "alien.hpp"
#include "basic_alien.hpp"
#include "fast_alien.hpp"
#include "tank_alien.hpp"
#include "elite_alien.hpp"
#include "boss_alien.hpp"
#include "mini_boss_alien.hpp"
#include "launcher_alien.hpp"
#include "beam_alien.hpp"
#include "anchored_elite.hpp"
#include "rocket_boss.hpp"
#include "led-matrix.h"
#include "input.hpp"
using namespace rgb_matrix;

const int MAX_ENEMIES = 100;
const int MAX_EXPLOSIONS = 10;
const int MAX_LEVEL = 6;
const int MAX_ENEMY_BULLETS = 50;

struct Explosion {
    int x, y;
    int timer;
    bool active;
};

const int MAX_WAVES = 10;

struct Wave {
    int basicCount;
    int fastCount;
    int tankCount;
    int eliteCount;
    int launcherCount;
    int beamCount;
    int anchoredEliteCount;      // Anchored elites (normal shooting)
    int anchoredEliteBeamCount;  // Anchored elites (beam mode)
    int rocketBossCount;
    int miniBossCount;
    int spawnRate;        // Default ticks between spawns
    int delayBefore;      // Ticks to wait before this wave starts (0 = no delay)
    // Per-type spawn rates (-1 = use default spawnRate)
    int basicRate;
    int fastRate;
    int tankRate;
    int eliteRate;
    int launcherRate;
    int beamRate;
    int anchoredEliteRate;
    int anchoredEliteBeamRate;
    int rocketBossRate;
    int miniBossRate;
};

struct LevelConfig {
    Wave waves[MAX_WAVES];
    int numWaves;
};

enum GameState {
    MAIN_MENU,
    LEVEL_SELECT,
    PLAYING,
    PLAYER_DYING,
    PAUSED,
    VICTORY,
    GAME_OVER
};

class Game{
public:
    Game();
    ~Game();
    void update(const InputState& input, RGBMatrix *matrix, int clock);
    void setup();
private:
    void drawHUD(RGBMatrix *matrix, int potentiometer);
    void drawLevelDisplay(RGBMatrix *matrix, int clock);
    void spawnEnemy(int clock);
    void updateEnemies(int clock);
    void drawEnemies(RGBMatrix *matrix);
    void eraseEnemies(RGBMatrix *matrix);
    void checkCollisions(int clock);
    void addExplosion(int x, int y);
    void updateExplosions();
    void drawExplosions(RGBMatrix *matrix);
    void eraseExplosions(RGBMatrix *matrix);
    void advanceLevel();
    void loadLevels(const char* filename);
    AlienType selectWaveAlienType();
    void updateEnemyBullets();
    void drawEnemyBullets(RGBMatrix *matrix);
    void eraseEnemyBullets(RGBMatrix *matrix);
    void checkEnemyBulletCollisions();
    void drawMainMenu(RGBMatrix *matrix);
    void drawLevelSelectMenu(RGBMatrix *matrix);
    void updateMenu(const InputState& input);
    void drawText(RGBMatrix *matrix, const char* text, int x, int y, int r, int g, int b);
    void drawBossHealthBar(RGBMatrix *matrix);
    void drawVictoryScreen(RGBMatrix *matrix);
    void drawGameOverScreen(RGBMatrix *matrix);

    Ship player;
    bool bossActive;
    bool bossSwarmSpawned;
    int bossSwarmRemaining;
    int playerDeathTimer;     // Explosion sequence when player dies
    Alien* enemies[MAX_ENEMIES];
    Bullet enemyBullets[MAX_ENEMY_BULLETS];
    Explosion explosions[MAX_EXPLOSIONS];
    int lastSpawnTime;
    int lastSpawnBasic;
    int lastSpawnFast;
    int lastSpawnTank;
    int lastSpawnElite;
    int lastSpawnLauncher;
    int lastSpawnBeam;
    int lastSpawnAnchoredElite;
    int lastSpawnAnchoredEliteBeam;
    int lastSpawnRocketBoss;
    int lastSpawnMiniBoss;
    int lastLaserDamageTick[100];  // Track last damage tick for each enemy
    int currentLevel;
    int levelDisplayTimer;  // Timer for level display
    int levelStartDelay;  // Delay before level starts (5 seconds)
    bool levelActive;  // Whether enemies should spawn
    LevelConfig levels[MAX_LEVEL];

    // Wave tracking
    int currentWave;          // Index of current wave within level
    int waveSpawnedBasic;     // How many basic aliens spawned in current wave
    int waveSpawnedFast;
    int waveSpawnedTank;
    int waveSpawnedElite;
    int waveSpawnedLauncher;
    int waveSpawnedBeam;
    int waveSpawnedAnchoredElite;
    int waveSpawnedAnchoredEliteBeam;
    int waveSpawnedRocketBoss;
    int waveSpawnedMiniBoss;
    bool allWavesComplete;
    int waveDelayTimer;       // Countdown timer for delay between waves
    GameState gameState;
    int menuSelection;  // 0=Play, 1=Level Select, 2=Quit
    int levelSelectChoice;  // Selected level (1-5)
    bool lastButton1;  // For button press detection
    int lastJoystickY;  // For joystick debouncing
    int joystickDebounceTimer;  // Timer to prevent rapid menu changes
};

#endif