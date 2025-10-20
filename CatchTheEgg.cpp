#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

using namespace std;

// Window dimensions
const int WINDOW_WIDTH = 900;
const int WINDOW_HEIGHT = 700;

// Game states
enum GameState { MENU, HELP, PLAYING, PAUSED, GAME_OVER };
GameState currentState = MENU;

// Game variables
int score = 0;
int highScore = 0;
float gameTime = 60.0f;
float timeRemaining = gameTime;
float lastTime = 0;
int comboCount = 0;
int maxCombo = 0;

// Basket properties
float basketX = 450;
float basketY = 50;
float basketWidth = 80;
float basketHeight = 60;
float basketSpeed = 15.0f;

// Multiple chicken properties
struct Chicken {
    float x, y;
    float speed;
    int direction;
    float stickY;
    float stickLeft, stickRight;
    bool active;
};

vector<Chicken> chickens;

// Egg/Item types
enum ItemType { 
    NORMAL_EGG, BLUE_EGG, GOLDEN_EGG, POOP, 
    PERK_LARGER_BASKET, PERK_SLOW_TIME, PERK_EXTRA_TIME,
    PERK_SHIELD, PERK_MAGNET, PERK_SPEED_BOOST,
    BOMB // New damage item
};

struct FallingItem {
    float x, y;
    float vx, vy; // Velocity for airflow
    ItemType type;
    float speed;
    bool active;
    float rotation;
    int sourceChicken; // Which chicken dropped it
};

vector<FallingItem> items;
float spawnTimer = 0;
float spawnInterval = 1.2f;

// Airflow system
struct Airflow {
    float startTime;
    float duration;
    float strength;
    int direction; // -1 left, 1 right
    bool active;
};

Airflow currentAirflow;
float airflowTimer = 0;
float airflowInterval = 8.0f;

// Particle system for effects
struct Particle {
    float x, y;
    float vx, vy;
    float life;
    float r, g, b;
};

vector<Particle> particles;

// Perk effects
float perkTimer = 0;
bool largerBasketActive = false;
bool slowTimeActive = false;
bool shieldActive = false;
bool magnetActive = false;
bool speedBoostActive = false;
float perkDuration = 5.0f;
int shieldHits = 0;

// Input
bool keys[256];
int mouseX = 0;

// Animation
float animTime = 0;

// Function declarations
void init();
void display();
void reshape(int w, int h);
void timer(int value);
void keyboard(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void mouseMotion(int x, int y);
void updateGame(float deltaTime);
void spawnItem(int chickenIndex);
void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18);
void drawMenu();
void drawHelp();
void drawGame();
void drawPaused();
void drawGameOver();
void drawBasket();
void drawChicken(Chicken& chicken);
void drawItem(FallingItem& item);
void drawStick(Chicken& chicken);
void drawAirflowIndicator();
void drawParticles();
bool checkCollision(FallingItem& item);
void resetGame();
void initChickens();
void createParticleExplosion(float x, float y, float r, float g, float b);
void playSound(int frequency); // Simple sound
void drawCircle(float cx, float cy, float r, int segments = 30);


void init() {
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    srand(time(0));
    
    // Initialize chickens
    initChickens();
    
    // Initialize airflow
    currentAirflow.active = false;
}





void playSound(int frequency) {
    // On Windows, this uses the console beep
    // On Linux/Mac, this might not work - it's just for demonstration
    #ifdef _WIN32
    Beep(frequency, 50);
    #else
    // On Unix systems, write to /dev/console (requires permissions)
    // Or use external command: system("beep -f frequency -l 50");
    cout << "\a"; // Terminal bell as fallback
    #endif
}



void drawChicken(Chicken& chicken) {
    if (!chicken.active) return;
    
    float bobbing = sin(animTime * 5) * 2;
    float cx = chicken.x;
    float cy = chicken.y + bobbing;
    
    // Body (white)
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(cx, cy, 20);
    
    // Head
    drawCircle(cx, cy + 20, 12);
    
    // Beak (yellow)
    glColor3f(1.0f, 0.8f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx + (chicken.direction * 12), cy + 20);
    glVertex2f(cx + (chicken.direction * 20), cy + 18);
    glVertex2f(cx + (chicken.direction * 12), cy + 16);
    glEnd();
    
    // Eye (black)
    glColor3f(0.0f, 0.0f, 0.0f);
    drawCircle(cx + (chicken.direction * 4), cy + 23, 2);
    
    // Comb (red) - animated
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - 3, cy + 28);
    glVertex2f(cx, cy + 35 + bobbing);
    glVertex2f(cx + 3, cy + 28);
    glEnd();
    
    // Wings (animated flapping)
    glColor3f(0.9f, 0.9f, 0.9f);
    float wingFlap = sin(animTime * 8) * 5;
    glBegin(GL_TRIANGLES);
    // Left wing
    glVertex2f(cx - 15, cy);
    glVertex2f(cx - 25 - wingFlap, cy + 5);
    glVertex2f(cx - 15, cy + 10);
    // Right wing
    glVertex2f(cx + 15, cy);
    glVertex2f(cx + 25 + wingFlap, cy + 5);
    glVertex2f(cx + 15, cy + 10);
    glEnd();
    
    // Legs
    glColor3f(1.0f, 0.8f, 0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glVertex2f(cx - 8, cy - 20);
    glVertex2f(cx - 8, cy - 5);
    glVertex2f(cx + 8, cy - 20);
    glVertex2f(cx + 8, cy - 5);
    glEnd();
}


