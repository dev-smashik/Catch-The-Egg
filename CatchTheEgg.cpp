#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/* Screen */
int WIN_W = 600;
int WIN_H = 700;

/* Game states */
typedef enum { STATE_MENU, STATE_PLAYING, STATE_PAUSED, STATE_GAMEOVER } GameState;
GameState state = STATE_MENU;

/* Timing */
const int FPS = 60;
const int TIMER_MS = 1000 / FPS;
const int TIME_LIMIT_SECONDS = 60;
int time_remaining = TIME_LIMIT_SECONDS;

/* Basket */
float basket_x = 300.0f;
const float BASKET_Y = 50.0f;
const float BASKET_WIDTH = 90.0f;
const float BASKET_HEIGHT = 25.0f;

/* Score */
int score = 0;
int highscore = 0;

/* Chicken */
float chicken_x = 300.0f;
float chicken_y = 620.0f;
float chicken_speed = 80.0f;

/* Speed progression */
int game_elapsed_ms = 0;
int speed_stage = 0;
const float SPEED_INCREASE = 1.25f;

/* Falling objects */
typedef enum { EGG_NORMAL, EGG_BLUE, EGG_GOLD, POOP } ObjType;

typedef struct {
    int active;
    float x, y, vy;
    ObjType type;
} FallingObj;

#define MAX_OBJS 30
FallingObj objs[MAX_OBJS];

int spawn_timer = 0;
int spawn_interval = 900;
float speed_multiplier = 1.0f;

/* Utility functions */
int rnd(int a, int b) { return a + rand() % (b - a + 1); }

void drawText(float x, float y, const char *s) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s++);
}

void drawTextCentered(float y, const char *s) {
    int len = 0;
    const char *p = s;
    while (*p++) len++;
    float x = (WIN_W - len * 10) / 2.0f;
    drawText(x, y, s);
}

/* Draw gradient background */
void draw_gradient_bg(float r1, float g1, float b1, float r2, float g2, float b2) {
    glBegin(GL_QUADS);
    glColor3f(r1, g1, b1);
    glVertex2f(0, 0);
    glVertex2f(WIN_W, 0);
    glColor3f(r2, g2, b2);
    glVertex2f(WIN_W, WIN_H);
    glVertex2f(0, WIN_H);
    glEnd();
}

/* Draw chicken */
void draw_chicken(float cx, float cy) {
    /* Body */
    glColor3f(1.0f, 0.95f, 0.2f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 32; ++i) {
        float theta = (2.0f * 3.14159f * i) / 32.0f;
        glVertex2f(cx + cosf(theta) * 24.0f, cy + sinf(theta) * 18.0f);
    }
    glEnd();
    
    /* Eye */
    glColor3f(0, 0, 0);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glVertex2f(cx + 8, cy + 8);
    glEnd();
    
    /* Beak */
    glColor3f(1.0f, 0.5f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx + 24, cy);
    glVertex2f(cx + 36, cy + 6);
    glVertex2f(cx + 36, cy - 6);
    glEnd();
    
    /* Comb */
    glColor3f(1.0f, 0.2f, 0.2f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - 4, cy + 18);
    glVertex2f(cx + 4, cy + 28);
    glVertex2f(cx + 8, cy + 18);
    glEnd();
}

/* Draw bamboo perch */
void draw_bamboo() {
    glColor3f(0.5f, 0.35f, 0.15f);
    glLineWidth(10.0f);
    glBegin(GL_LINES);
    glVertex2f(30, chicken_y - 15);
    glVertex2f(WIN_W - 30, chicken_y - 15);
    glEnd();
    
    /* Bamboo segments */
    glColor3f(0.4f, 0.3f, 0.1f);
    glLineWidth(2.0f);
    for (int i = 80; i < WIN_W - 80; i += 60) {
        glBegin(GL_LINES);
        glVertex2f(i, chicken_y - 20);
        glVertex2f(i, chicken_y - 10);
        glEnd();
    }
}

/* Draw basket */
void draw_basket() {
    float x = basket_x;
    float y = BASKET_Y;
    float w = BASKET_WIDTH / 2;
    
    /* Shadow */
    glColor4f(0, 0, 0, 0.2f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 20; ++i) {
        float angle = 3.14159f * i / 20.0f;
        glVertex2f(x + cosf(angle) * (w + 10), y - BASKET_HEIGHT - 2);
    }
    glEnd();
    
    /* Basket body */
    glColor3f(0.85f, 0.4f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(x - w - 8, y);
    glVertex2f(x + w + 8, y);
    glVertex2f(x + w, y - BASKET_HEIGHT);
    glVertex2f(x - w, y - BASKET_HEIGHT);
    glEnd();
    
    /* Rim */
    glColor3f(0.7f, 0.3f, 0.05f);
    glLineWidth(4.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x - w - 8, y);
    glVertex2f(x - w, y + 6);
    glVertex2f(x + w, y + 6);
    glVertex2f(x + w + 8, y);
    glEnd();
    
    /* Weave pattern */
    glColor3f(0.6f, 0.25f, 0.05f);
    glLineWidth(1.5f);
    for (int i = -3; i <= 3; ++i) {
        glBegin(GL_LINES);
        glVertex2f(x + i * 12, y - 2);
        glVertex2f(x + i * 12, y - BASKET_HEIGHT + 2);
        glEnd();
    }
}

/* Draw falling object with glow effect */
void draw_obj(const FallingObj *o) {
    if (!o->active) return;
    
    /* Glow effect for special eggs */
    if (o->type == EGG_GOLD || o->type == EGG_BLUE) {
        glColor4f(1.0f, 1.0f, 0.5f, 0.3f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 24; ++i) {
            float th = (2.0f * 3.14159f * i) / 24.0f;
            glVertex2f(o->x + cosf(th) * 14, o->y + sinf(th) * 18);
        }
        glEnd();
    }
    
    /* Object color */
    switch (o->type) {
        case EGG_NORMAL: glColor3f(1.0f, 1.0f, 0.95f); break;
        case EGG_BLUE: glColor3f(0.4f, 0.7f, 1.0f); break;
        case EGG_GOLD: glColor3f(1.0f, 0.85f, 0.2f); break;
        case POOP: glColor3f(0.3f, 0.2f, 0.1f); break;
    }
    
    /* Draw egg/poop */
    glBegin(GL_POLYGON);
    for (int i = 0; i < 24; ++i) {
        float th = (2.0f * 3.14159f * i) / 24.0f;
        glVertex2f(o->x + cosf(th) * 9, o->y + sinf(th) * 13);
    }
    glEnd();
    
    /* Highlight */
    if (o->type != POOP) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 12; ++i) {
            float th = (2.0f * 3.14159f * i) / 12.0f;
            glVertex2f(o->x - 3 + cosf(th) * 3, o->y + 4 + sinf(th) * 4);
        }
        glEnd();
    }
}

/* Draw modern HUD */
void draw_hud() {
    char buf[64];
    
    /* Score panel */
    glColor4f(0.1f, 0.1f, 0.2f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(10, WIN_H - 70);
    glVertex2f(180, WIN_H - 70);
    glVertex2f(180, WIN_H - 10);
    glVertex2f(10, WIN_H - 10);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "SCORE: %d", score);
    drawText(20, WIN_H - 25, buf);
    sprintf(buf, "HIGH: %d", highscore);
    drawText(20, WIN_H - 50, buf);
    
    /* Time panel */
    glColor4f(0.1f, 0.1f, 0.2f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(WIN_W - 150, WIN_H - 50);
    glVertex2f(WIN_W - 10, WIN_H - 50);
    glVertex2f(WIN_W - 10, WIN_H - 10);
    glVertex2f(WIN_W - 150, WIN_H - 10);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "TIME: %02d", time_remaining);
    drawText(WIN_W - 130, WIN_H - 25, buf);
}

void reset_game() {
    score = 0;
    time_remaining = TIME_LIMIT_SECONDS;
    speed_multiplier = 1.0f;
    chicken_x = WIN_W / 2;
    chicken_y = 620.0f;
    chicken_speed = 80.0f;
    basket_x = WIN_W / 2;
    game_elapsed_ms = 0;
    speed_stage = 0;
    spawn_timer = 0;
    for (int i = 0; i < MAX_OBJS; ++i) objs[i].active = 0;
}

void spawn_object() {
    for (int i = 0; i < MAX_OBJS; ++i) {
        if (!objs[i].active) {
            objs[i].active = 1;
            objs[i].x = chicken_x + rnd(-20, 20);
            objs[i].y = chicken_y - 20;
            
            int r = rnd(1, 100);
            if (r <= 5) {
                objs[i].type = EGG_GOLD;
                objs[i].vy = 110.0f;
            } else if (r <= 18) {
                objs[i].type = EGG_BLUE;
                objs[i].vy = 100.0f;
            } else if (r <= 28) {
                objs[i].type = POOP;
                objs[i].vy = 130.0f;
            } else {
                objs[i].type = EGG_NORMAL;
                objs[i].vy = 90.0f;
            }
            break;
        }
    }
}

void check_collision(FallingObj *o) {
    if (!o->active) return;
    
    if (o->y <= BASKET_Y + 6 && o->y >= BASKET_Y - BASKET_HEIGHT) {
        float left = basket_x - BASKET_WIDTH / 2 - 8;
        float right = basket_x + BASKET_WIDTH / 2 + 8;
        
        if (o->x >= left && o->x <= right) {
            switch (o->type) {
                case EGG_NORMAL: score += 1; break;
                case EGG_BLUE: score += 5; break;
                case EGG_GOLD: score += 10; break;
                case POOP: score -= 10; break;
            }
            if (score > highscore) highscore = score;
            o->active = 0;
        }
    }
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (state == STATE_MENU) {
        draw_gradient_bg(0.5f, 0.8f, 1.0f, 0.3f, 0.5f, 0.8f);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawTextCentered(WIN_H - 120, "EGG CATCHER");
        
        glColor3f(0.2f, 0.2f, 0.3f);
        drawTextCentered(WIN_H / 2 + 60, "Press S to Start");
        drawTextCentered(WIN_H / 2 + 30, "Move: Mouse or Arrow Keys or A/D");
        drawTextCentered(WIN_H / 2, "Pause: P");
        drawTextCentered(WIN_H / 2 - 30, "Quit: Q or ESC");
        drawTextCentered(WIN_H / 2 - 70, "Catch eggs, avoid poop!");
        
        char buf[64];
        sprintf(buf, "High Score: %d", highscore);
        drawTextCentered(WIN_H / 2 - 110, buf);
        
    } else if (state == STATE_PLAYING || state == STATE_PAUSED) {
        draw_gradient_bg(0.5f, 0.85f, 1.0f, 0.7f, 0.95f, 1.0f);
        
        draw_bamboo();
        draw_chicken(chicken_x, chicken_y);
        
        for (int i = 0; i < MAX_OBJS; ++i) {
            if (objs[i].active) draw_obj(&objs[i]);
        }
        
        draw_basket();
        draw_hud();
        
        if (state == STATE_PAUSED) {
            glColor4f(0, 0, 0, 0.6f);
            glBegin(GL_QUADS);
            glVertex2f(0, 0);
            glVertex2f(WIN_W, 0);
            glVertex2f(WIN_W, WIN_H);
            glVertex2f(0, WIN_H);
            glEnd();
            
            glColor3f(1.0f, 1.0f, 1.0f);
            drawTextCentered(WIN_H / 2 + 20, "PAUSED");
            drawTextCentered(WIN_H / 2 - 20, "Press P to Resume");
        }
        
    } else if (state == STATE_GAMEOVER) {
        draw_gradient_bg(0.2f, 0.2f, 0.3f, 0.1f, 0.1f, 0.2f);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        drawTextCentered(WIN_H / 2 + 80, "GAME OVER");
        
        char buf[64];
        sprintf(buf, "Final Score: %d", score);
        drawTextCentered(WIN_H / 2 + 40, buf);
        sprintf(buf, "High Score: %d", highscore);
        drawTextCentered(WIN_H / 2 + 10, buf);
        
        glColor3f(0.8f, 0.8f, 0.9f);
        drawTextCentered(WIN_H / 2 - 40, "S: Restart   M: Menu   Q: Quit");
    }
    
    glutSwapBuffers();
}

void update_game(int value) {
    static int last_time = 0;
    static int init = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (!init) { last_time = now; init = 1; }
    int dt = now - last_time;
    if (dt > 1000) dt = TIMER_MS;
    last_time = now;
    
    if (state == STATE_PLAYING) {
        game_elapsed_ms += dt;
        
        /* Speed increases at 15s, 30s, 45s */
        if (speed_stage == 0 && game_elapsed_ms >= 15000) {
            chicken_speed *= SPEED_INCREASE;
            speed_multiplier *= SPEED_INCREASE;
            speed_stage = 1;
        } else if (speed_stage == 1 && game_elapsed_ms >= 30000) {
            chicken_speed *= SPEED_INCREASE;
            speed_multiplier *= SPEED_INCREASE;
            speed_stage = 2;
        } else if (speed_stage == 2 && game_elapsed_ms >= 45000) {
            chicken_speed *= SPEED_INCREASE;
            speed_multiplier *= SPEED_INCREASE;
            speed_stage = 3;
        }
        
        /* Move chicken */
        float dt_sec = dt / 1000.0f;
        chicken_x += chicken_speed * dt_sec;
        if (chicken_x < 50) {
            chicken_x = 50;
            chicken_speed = fabs(chicken_speed);
        }
        if (chicken_x > WIN_W - 50) {
            chicken_x = WIN_W - 50;
            chicken_speed = -fabs(chicken_speed);
        }
        
        /* Spawn objects */
        spawn_timer += dt;
        int interval = spawn_interval;
        if (score >= 30) interval = 550;
        else if (score >= 15) interval = 730;
        
        if (spawn_timer >= interval) {
            spawn_timer = 0;
            spawn_object();
        }
        
        /* Update falling objects */
        for (int i = 0; i < MAX_OBJS; ++i) {
            if (!objs[i].active) continue;
            objs[i].y -= objs[i].vy * dt_sec * speed_multiplier;
            check_collision(&objs[i]);
            if (objs[i].y < -20) objs[i].active = 0;
        }
        
        /* Timer countdown */
        static int time_acc = 0;
        time_acc += dt;
        if (time_acc >= 1000) {
            time_acc -= 1000;
            time_remaining--;
            if (time_remaining <= 0) {
                time_remaining = 0;
                state = STATE_GAMEOVER;
            }
        }
    }
    
    glutPostRedisplay();
    glutTimerFunc(TIMER_MS, update_game, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (state == STATE_MENU) {
        if (key == 's' || key == 'S') {
            reset_game();
            state = STATE_PLAYING;
        } else if (key == 'q' || key == 'Q' || key == 27) exit(0);
    } else if (state == STATE_PLAYING) {
        if (key == 'p' || key == 'P') state = STATE_PAUSED;
        else if (key == 'q' || key == 'Q' || key == 27) exit(0);
        else if (key == 'a' || key == 'A') {
            basket_x -= 25;
            if (basket_x < 50) basket_x = 50;
        } else if (key == 'd' || key == 'D') {
            basket_x += 25;
            if (basket_x > WIN_W - 50) basket_x = WIN_W - 50;
        }
    } else if (state == STATE_PAUSED) {
        if (key == 'p' || key == 'P') state = STATE_PLAYING;
        else if (key == 'q' || key == 'Q' || key == 27) exit(0);
        else if (key == 'm' || key == 'M') state = STATE_MENU;
    } else if (state == STATE_GAMEOVER) {
        if (key == 's' || key == 'S') {
            reset_game();
            state = STATE_PLAYING;
        } else if (key == 'm' || key == 'M') state = STATE_MENU;
        else if (key == 'q' || key == 'Q' || key == 27) exit(0);
    }
}

void special_keys(int key, int x, int y) {
    if (state == STATE_PLAYING) {
        if (key == GLUT_KEY_LEFT) {
            basket_x -= 25;
            if (basket_x < 50) basket_x = 50;
        }
        if (key == GLUT_KEY_RIGHT) {
            basket_x += 25;
            if (basket_x > WIN_W - 50) basket_x = WIN_W - 50;
        }
    }
}

void passive_mouse(int x, int y) {
    basket_x = (float)x;
    if (basket_x < 50) basket_x = 50;
    if (basket_x > WIN_W - 50) basket_x = WIN_W - 50;
}

void reshape(int w, int h) {
    WIN_W = w;
    WIN_H = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void init_gl() {
    glClearColor(0.5f, 0.85f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

int main(int argc, char **argv) {
    srand((unsigned int)time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Egg Catcher - Improved");
    init_gl();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);
    glutPassiveMotionFunc(passive_mouse);
    glutTimerFunc(TIMER_MS, update_game, 0);
    reset_game();
    glutMainLoop();
    return 0;
}
