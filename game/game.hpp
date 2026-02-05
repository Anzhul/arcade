#ifndef GAME_HPP
#define GAME_HPP
#include "ship.hpp"
#include "alien.hpp"
#include "basic_alien.hpp"
#include "fast_alien.hpp"
#include "tank_alien.hpp"
#include "elite_alien.hpp"
#include "led-matrix.h"
#include "input.hpp"
using namespace rgb_matrix;

const int MAX_ENEMIES = 100;
const int MAX_EXPLOSIONS = 10;
const int MAX_LEVEL = 5;
const int MAX_ENEMY_BULLETS = 50;

struct Explosion {
    int x, y;
    int timer;
    bool active;
};

struct LevelConfig {
    int spawnRate;           // Ticks between spawns
    int basicPercent;        // Percentage of basic aliens
    int fastPercent;         // Percentage of fast aliens
    int tankPercent;         // Percentage of tank aliens
    int elitePercent;        // Percentage of elite aliens
    int enemiesToKill;       // Number of enemies to kill to complete level
};

enum GameState {
    MAIN_MENU,
    LEVEL_SELECT,
    PLAYING,
    PAUSED
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
    AlienType selectAlienType();
    void updateEnemyBullets();
    void drawEnemyBullets(RGBMatrix *matrix);
    void eraseEnemyBullets(RGBMatrix *matrix);
    void checkEnemyBulletCollisions();
    void drawMainMenu(RGBMatrix *matrix);
    void drawLevelSelectMenu(RGBMatrix *matrix);
    void updateMenu(const InputState& input);
    void drawText(RGBMatrix *matrix, const char* text, int x, int y, int r, int g, int b);
    
    Ship player;
    Alien* enemies[MAX_ENEMIES];
    Bullet enemyBullets[MAX_ENEMY_BULLETS];
    Explosion explosions[MAX_EXPLOSIONS];
    int lastSpawnTime;
    int lastLaserDamageTick[100];  // Track last damage tick for each enemy
    int currentLevel;
    int enemiesKilled;
    int levelDisplayTimer;  // Timer for level display
    int levelStartDelay;  // Delay before level starts (5 seconds)
    bool levelActive;  // Whether enemies should spawn
    LevelConfig levels[MAX_LEVEL];
    GameState gameState;
    int menuSelection;  // 0=Play, 1=Level Select, 2=Quit
    int levelSelectChoice;  // Selected level (1-5)
    bool lastButton1;  // For button press detection
    int lastJoystickY;  // For joystick debouncing
    int joystickDebounceTimer;  // Timer to prevent rapid menu changes
};

#endif