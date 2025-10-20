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

void drawAirflowIndicator() {
    if (!currentAirflow.active) return;
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    int numLines = 8;
    for (int i = 0; i < numLines; i++) {
        float y = 100 + (i * 60);
        float offset = fmod(animTime * 100 * currentAirflow.direction, 50);
        
        for (int j = 0; j < 20; j++) {
            float x = j * 50 + offset;
            glBegin(GL_LINES);
            glVertex2f(x, y);
            glVertex2f(x + 30 * currentAirflow.direction, y);
            glEnd();
        }
    }
    
    glDisable(GL_BLEND);
    
    // Text indicator
    glColor3f(1.0f, 1.0f, 1.0f);
    const char* text = currentAirflow.direction > 0 ? "WIND >>>" : "<<< WIND";
    drawText(WINDOW_WIDTH/2 - 40, 600, text, GLUT_BITMAP_HELVETICA_18);
}



void updateGame(float deltaTime) {
    if (currentState != PLAYING) return;
    
    animTime += deltaTime;
    
    // Update time
    timeRemaining -= deltaTime;
    if (timeRemaining <= 0) {
        timeRemaining = 0;
        currentState = GAME_OVER;
        if (score > highScore) {
            highScore = score;
        }
        return;
    }
    
    // Activate third chicken after 20 seconds
    if (gameTime - timeRemaining > 20 && chickens.size() > 2) {
        chickens[2].active = true;
    }
    
    // Update perk timer
    if (perkTimer > 0) {
        perkTimer -= deltaTime;
        if (perkTimer <= 0) {
            largerBasketActive = false;
            slowTimeActive = false;
            shieldActive = false;
            magnetActive = false;
            speedBoostActive = false;
            basketWidth = 80;
            basketSpeed = 15.0f;
        }
    }
    
    // Update airflow
    airflowTimer += deltaTime;
    if (airflowTimer > airflowInterval) {
        currentAirflow.active = true;
        currentAirflow.duration = 5.0f;
        currentAirflow.strength = 2.0f + (rand() % 3);
        currentAirflow.direction = (rand() % 2) * 2 - 1;
        currentAirflow.startTime = animTime;
        airflowTimer = 0;
        playSound(400);
    }
    
    if (currentAirflow.active) {
        if (animTime - currentAirflow.startTime > currentAirflow.duration) {
            currentAirflow.active = false;
        }
    }
    
    // Update chickens
    for (auto& chicken : chickens) {
        if (!chicken.active) continue;
        
        chicken.x += chicken.speed * chicken.direction;
        if (chicken.x > chicken.stickRight - 30 || chicken.x < chicken.stickLeft + 30) {
            chicken.direction *= -1;
        }
    }
    
    // Move basket
    float currentSpeed = speedBoostActive ? basketSpeed * 2 : basketSpeed;
    if (keys['a'] || keys['A']) {
        basketX -= currentSpeed;
    }
    if (keys['d'] || keys['D']) {
        basketX += currentSpeed;
    }
    
    if (basketX < basketWidth/2) basketX = basketWidth/2;
    if (basketX > WINDOW_WIDTH - basketWidth/2) basketX = WINDOW_WIDTH - basketWidth/2;
    
    // Spawn items
    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval) {
        for (int i = 0; i < chickens.size(); i++) {
            if (chickens[i].active && (rand() % 2)) {
                spawnItem(i);
            }
        }
        spawnTimer = 0;
    }
    
    // Update items
    for (auto it = items.begin(); it != items.end();) {
        it->y -= it->speed;
        it->rotation += 2.0f;
        
        // Apply airflow
        if (currentAirflow.active) {
            it->vx += currentAirflow.strength * currentAirflow.direction * 0.1f;
        }
        
        // Apply velocity with damping
        it->x += it->vx;
        it->vx *= 0.95f;
        
        // Check collision
        if (checkCollision(*it)) {
            bool caught = true;
            
            switch (it->type) {
                case NORMAL_EGG:
                    score += 1 + comboCount;
                    comboCount++;
                    createParticleExplosion(it->x, it->y, 1.0f, 1.0f, 1.0f);
                    playSound(600);
                    break;
                case BLUE_EGG:
                    score += 5 + comboCount * 2;
                    comboCount++;
                    createParticleExplosion(it->x, it->y, 0.3f, 0.5f, 0.9f);
                    playSound(800);
                    break;
                case GOLDEN_EGG:
                    score += 10 + comboCount * 3;
                    comboCount++;
                    createParticleExplosion(it->x, it->y, 1.0f, 0.84f, 0.0f);
                    playSound(1000);
                    break;
                case POOP:
                    if (shieldActive) {
                        shieldHits++;
                        if (shieldHits >= 3) {
                            shieldActive = false;
                            perkTimer = 0;
                        }
                    } else {
                        score -= 10;
                        if (score < 0) score = 0;
                        comboCount = 0;
                    }
                    createParticleExplosion(it->x, it->y, 0.4f, 0.3f, 0.2f);
                    playSound(200);
                    break;
                case BOMB:
                    if (shieldActive) {
                        shieldHits++;
                        if (shieldHits >= 3) {
                            shieldActive = false;
                            perkTimer = 0;
                        }
                    } else {
                        score -= 20;
                        if (score < 0) score = 0;
                        comboCount = 0;
                        // Explosion effect
                        for (int i = 0; i < 30; i++) {
                            createParticleExplosion(it->x, it->y, 1.0f, 0.3f, 0.0f);
                        }
                    }
                    playSound(150);
                    break;
                case PERK_LARGER_BASKET:
                    largerBasketActive = true;
                    basketWidth = 120;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
                case PERK_SLOW_TIME:
                    slowTimeActive = true;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
                case PERK_EXTRA_TIME:
                    timeRemaining += 10.0f;
                    playSound(900);
                    break;
                case PERK_SHIELD:
                    shieldActive = true;
                    shieldHits = 0;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
                case PERK_MAGNET:
                    magnetActive = true;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
                case PERK_SPEED_BOOST:
                    speedBoostActive = true;
                    basketSpeed = 25.0f;
                    perkTimer = perkDuration;
                    playSound(900);
                    break;
            }
            
            if (comboCount > maxCombo) {
                maxCombo = comboCount;
            }
            
            it = items.erase(it);
        } else if (it->y < -30 || it->x < -50 || it->x > WINDOW_WIDTH + 50) {
            if (it->type == NORMAL_EGG || it->type == BLUE_EGG || it->type == GOLDEN_EGG) {
                comboCount = 0;
            }
            it = items.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update particles
    for (auto it = particles.begin(); it != particles.end();) {
        it->x += it->vx;
        it->y += it->vy;
        it->vy -= 0.2f; // Gravity
        it->life -= deltaTime * 2;
        
        if (it->life <= 0) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}




void keyboard(unsigned char key, int x, int y) {
    keys[key] = true;
    
    switch (currentState) {
        case MENU:
            if (key == 13) { // Enter
                resetGame();
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            } else if (key == 'h' || key == 'H') {
                currentState = HELP;
            } else if (key == 'q' || key == 'Q') {
                exit(0);
            }
            break;
            
        case HELP:
            if (key == 27) { // ESC
                currentState = MENU;
            }
            break;
            
        case PLAYING:
            if (key == 'p' || key == 'P') {
                currentState = PAUSED;
            } else if (key == 27) { // ESC
                currentState = MENU;
            }
            break;
            
        case PAUSED:
            if (key == 'p' || key == 'P') {
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            } else if (key == 27) { // ESC
                currentState = MENU;
            }
            break;
            
        case GAME_OVER:
            if (key == 13) { // Enter
                resetGame();
                currentState = PLAYING;
                lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            } else if (key == 27) { // ESC
                currentState = MENU;
            }
            break;
    }
}

// Keyboard up callback
void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// Mouse callback
void mouse(int button, int state, int x, int y) {
    if (currentState == PLAYING && button == GLUT_LEFT_BUTTON) {
        mouseX = x;
        basketX = x;
    }
}


// Mouse motion callback
void mouseMotion(int x, int y) {
    if (currentState == PLAYING) {
        basketX = x;
        if (basketX < basketWidth/2) basketX = basketWidth/2;
        if (basketX > WINDOW_WIDTH - basketWidth/2) basketX = WINDOW_WIDTH - basketWidth/2;
    }
}



int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Catch The Eggs - Enhanced Edition");
    
    init();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(mouseMotion);
    glutTimerFunc(0, timer, 0);
    
    cout << "\n";
    cout << "========================================\n";
    cout << "   CATCH THE EGGS  \n";
    cout << "========================================\n\n";
    cout << "FEATURES:\n";
    cout << "- Multiple chickens on different sticks\n";
    cout << "- Wind system affects falling items\n";
    cout << "- Shield, Magnet, Speed Boost perks\n";
    cout << "- Combo system for bonus points\n";
    cout << "- Particle effects and sound\n";
    cout << "- Bombs for extra challenge\n\n";
    cout << "Press H in menu for detailed help!\n";
    cout << "========================================\n\n";
    
    glutMainLoop();
    return 0;
}
