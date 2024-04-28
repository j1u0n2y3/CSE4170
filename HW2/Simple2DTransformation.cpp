#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "Shaders/LoadShaders.h"
GLuint h_ShaderProgram;                                   // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

// include glm/*.hpp only if necessary
// #include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, ortho, etc.
glm::mat4 ModelViewProjectionMatrix;
glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;

#define TO_RADIAN 0.01745329252f
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *)(offset))

#define LOC_VERTEX 0

int win_width = 0, win_height = 0;
float centerx = 0.0f, centery = 0.0f, rotate_angle = 0.0f;

int leftbuttonpressed = 0;
int clickedOnBall1 = 0, clickedOnBall2 = 0;
int prevx, prevy;
int mprevx, mprevy;
int dxo, dyo;
typedef struct
{
    float cx, cy;
    int exist;
} Big;
typedef struct
{
    float cx, cy;
    int exist;
} Small;
Big big[5];
Small small[5];
// Need : fric=weight, cx, cy, rad, vx, vy, weight=rad^3 << 3 balls

// Object 5

// cake
#define CAKE_FIRE 0
#define CAKE_CANDLE 1
#define CAKE_BODY 2
#define CAKE_BOTTOM 3
#define CAKE_DECORATE 4

GLfloat cake_fire[4][2] = {{-0.5, 14.0}, {-0.5, 13.0}, {0.5, 13.0}, {0.5, 14.0}};
GLfloat cake_candle[4][2] = {{-1.0, 8.0}, {-1.0, 13.0}, {1.0, 13.0}, {1.0, 8.0}};
GLfloat cake_body[4][2] = {{8.0, 5.0}, {-8.0, 5.0}, {-8.0, 8.0}, {8.0, 8.0}};
GLfloat cake_bottom[4][2] = {{-10.0, 1.0}, {-10.0, 5.0}, {10.0, 5.0}, {10.0, 1.0}};
GLfloat cake_decorate[4][2] = {{-10.0, 0.0}, {-10.0, 1.0}, {10.0, 1.0}, {10.0, 0.0}};

GLfloat cake_color[5][3] = {
    {255 / 255.0f, 0 / 255.0f, 0 / 255.0f},
    {255 / 255.0f, 204 / 255.0f, 0 / 255.0f},
    {255 / 255.0f, 102 / 255.0f, 255 / 255.0f},
    {255 / 255.0f, 102 / 255.0f, 255 / 255.0f},
    {102 / 255.0f, 51 / 255.0f, 0 / 255.0f}};

GLuint VBO_cake, VAO_cake;

void prepare_cake()
{
    int size = sizeof(cake_fire);
    GLsizeiptr buffer_size = sizeof(cake_fire) * 5;

    // Initialize vertex buffer object.
    glGenBuffers(1, &VBO_cake);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cake);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

    glBufferSubData(GL_ARRAY_BUFFER, 0, size, cake_fire);
    glBufferSubData(GL_ARRAY_BUFFER, size, size, cake_candle);
    glBufferSubData(GL_ARRAY_BUFFER, size * 2, size, cake_body);
    glBufferSubData(GL_ARRAY_BUFFER, size * 3, size, cake_bottom);
    glBufferSubData(GL_ARRAY_BUFFER, size * 4, size, cake_decorate);

    // Initialize vertex array object.
    glGenVertexArrays(1, &VAO_cake);
    glBindVertexArray(VAO_cake);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cake);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void draw_cake()
{
    glBindVertexArray(VAO_cake);

    glUniform3fv(loc_primitive_color, 1, cake_color[CAKE_FIRE]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUniform3fv(loc_primitive_color, 1, cake_color[CAKE_CANDLE]);
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

    glUniform3fv(loc_primitive_color, 1, cake_color[CAKE_BODY]);
    glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

    glUniform3fv(loc_primitive_color, 1, cake_color[CAKE_BOTTOM]);
    glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

    glUniform3fv(loc_primitive_color, 1, cake_color[CAKE_DECORATE]);
    glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

    glBindVertexArray(0);
}

// airplane
#define AIRPLANE_BIG_WING 0
#define AIRPLANE_SMALL_WING 1
#define AIRPLANE_BODY 2
#define AIRPLANE_BACK 3
#define AIRPLANE_SIDEWINDER1 4
#define AIRPLANE_SIDEWINDER2 5
#define AIRPLANE_CENTER 6
GLfloat big_wing[6][2] = {{0.0, 0.0}, {-20.0, 15.0}, {-20.0, 20.0}, {0.0, 23.0}, {20.0, 20.0}, {20.0, 15.0}};
GLfloat small_wing[6][2] = {{0.0, -18.0}, {-11.0, -12.0}, {-12.0, -7.0}, {0.0, -10.0}, {12.0, -7.0}, {11.0, -12.0}};
GLfloat body[5][2] = {{0.0, -25.0}, {-6.0, 0.0}, {-6.0, 22.0}, {6.0, 22.0}, {6.0, 0.0}};
GLfloat back[5][2] = {{0.0, 25.0}, {-7.0, 24.0}, {-7.0, 21.0}, {7.0, 21.0}, {7.0, 24.0}};
GLfloat sidewinder1[5][2] = {{-20.0, 10.0}, {-18.0, 3.0}, {-16.0, 10.0}, {-18.0, 20.0}, {-20.0, 20.0}};
GLfloat sidewinder2[5][2] = {{20.0, 10.0}, {18.0, 3.0}, {16.0, 10.0}, {18.0, 20.0}, {20.0, 20.0}};
GLfloat center[1][2] = {{0.0, 0.0}};
GLfloat airplane_color[7][3] = {
    {150 / 255.0f, 129 / 255.0f, 183 / 255.0f}, // big_wing
    {245 / 255.0f, 211 / 255.0f, 0 / 255.0f},   // small_wing
    {111 / 255.0f, 85 / 255.0f, 157 / 255.0f},  // body
    {150 / 255.0f, 129 / 255.0f, 183 / 255.0f}, // back
    {245 / 255.0f, 211 / 255.0f, 0 / 255.0f},   // sidewinder1
    {245 / 255.0f, 211 / 255.0f, 0 / 255.0f},   // sidewinder2
    {255 / 255.0f, 0 / 255.0f, 0 / 255.0f}      // center
};

GLuint VBO_airplane, VAO_airplane;

int airplane_clock = 0;
float airplane_s_factor = 1.0f;

void prepare_airplane()
{
    int i;
    for (i = 0; i < 5; i++)
    {
        big[i].exist = 0;
    }
    GLsizeiptr buffer_size = sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back) + sizeof(sidewinder1) + sizeof(sidewinder2) + sizeof(center);

    // Initialize vertex buffer object.
    glGenBuffers(1, &VBO_airplane);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_airplane);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(big_wing), big_wing);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing), sizeof(small_wing), small_wing);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing), sizeof(body), body);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body), sizeof(back), back);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back),
                    sizeof(sidewinder1), sidewinder1);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back) + sizeof(sidewinder1), sizeof(sidewinder2), sidewinder2);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back) + sizeof(sidewinder1) + sizeof(sidewinder2), sizeof(center), center);

    // Initialize vertex array object.
    glGenVertexArrays(1, &VAO_airplane);
    glBindVertexArray(VAO_airplane);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_airplane);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void draw_airplane()
{ // Draw airplane in its MC.
    glBindVertexArray(VAO_airplane);

    glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BIG_WING]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

    glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SMALL_WING]);
    glDrawArrays(GL_TRIANGLE_FAN, 6, 6);

    glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BODY]);
    glDrawArrays(GL_TRIANGLE_FAN, 12, 5);

    glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BACK]);
    glDrawArrays(GL_TRIANGLE_FAN, 17, 5);

    glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SIDEWINDER1]);
    glDrawArrays(GL_TRIANGLE_FAN, 22, 5);

    glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SIDEWINDER2]);
    glDrawArrays(GL_TRIANGLE_FAN, 27, 5);

    glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_CENTER]);
    glPointSize(5.0);
    glDrawArrays(GL_POINTS, 32, 1);
    glPointSize(1.0);
    glBindVertexArray(0);
}

// cocktail
#define COCKTAIL_NECK 0
#define COCKTAIL_LIQUID 1
#define COCKTAIL_REMAIN 2
#define COCKTAIL_STRAW 3
#define COCKTAIL_DECO 4

GLfloat neck[6][2] = {{-6.0, -12.0}, {-6.0, -11.0}, {-1.0, 0.0}, {1.0, 0.0}, {6.0, -11.0}, {6.0, -12.0}};
GLfloat liquid[6][2] = {{-1.0, 0.0}, {-9.0, 4.0}, {-12.0, 7.0}, {12.0, 7.0}, {9.0, 4.0}, {1.0, 0.0}};
GLfloat remain[4][2] = {{-12.0, 7.0}, {-12.0, 10.0}, {12.0, 10.0}, {12.0, 7.0}};
GLfloat straw[4][2] = {{7.0, 7.0}, {12.0, 12.0}, {14.0, 12.0}, {9.0, 7.0}};
GLfloat deco[8][2] = {{12.0, 12.0}, {10.0, 14.0}, {10.0, 16.0}, {12.0, 18.0}, {14.0, 18.0}, {16.0, 16.0}, {16.0, 14.0}, {14.0, 12.0}};

GLfloat cocktail_color[5][3] = {
    {235 / 255.0f, 225 / 255.0f, 196 / 255.0f},
    {0 / 255.0f, 63 / 255.0f, 122 / 255.0f},
    {235 / 255.0f, 225 / 255.0f, 196 / 255.0f},
    {191 / 255.0f, 255 / 255.0f, 0 / 255.0f},
    {218 / 255.0f, 165 / 255.0f, 32 / 255.0f}};

GLuint VBO_cocktail, VAO_cocktail;
void prepare_cocktail()
{
    GLsizeiptr buffer_size = sizeof(neck) + sizeof(liquid) + sizeof(remain) + sizeof(straw) + sizeof(deco);

    // Initialize vertex buffer object.
    glGenBuffers(1, &VBO_cocktail);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cocktail);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(neck), neck);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck), sizeof(liquid), liquid);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid), sizeof(remain), remain);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid) + sizeof(remain), sizeof(straw), straw);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid) + sizeof(remain) + sizeof(straw),
                    sizeof(deco), deco);

    // Initialize vertex array object.
    glGenVertexArrays(1, &VAO_cocktail);
    glBindVertexArray(VAO_cocktail);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cocktail);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void draw_cocktail()
{
    glBindVertexArray(VAO_cocktail);

    glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_NECK]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

    glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_LIQUID]);
    glDrawArrays(GL_TRIANGLE_FAN, 6, 6);

    glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_REMAIN]);
    glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

    glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_STRAW]);
    glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

    glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_DECO]);
    glDrawArrays(GL_TRIANGLE_FAN, 20, 8);

    glBindVertexArray(0);
}

// shirt
#define SHIRT_LEFT_BODY 0
#define SHIRT_RIGHT_BODY 1
#define SHIRT_LEFT_COLLAR 2
#define SHIRT_RIGHT_COLLAR 3
#define SHIRT_FRONT_POCKET 4
#define SHIRT_BUTTON1 5
#define SHIRT_BUTTON2 6
#define SHIRT_BUTTON3 7
#define SHIRT_BUTTON4 8
GLfloat left_body[6][2] = {{0.0, -9.0}, {-8.0, -9.0}, {-11.0, 8.0}, {-6.0, 10.0}, {-3.0, 7.0}, {0.0, 9.0}};
GLfloat right_body[6][2] = {{0.0, -9.0}, {0.0, 9.0}, {3.0, 7.0}, {6.0, 10.0}, {11.0, 8.0}, {8.0, -9.0}};
GLfloat left_collar[4][2] = {{0.0, 9.0}, {-3.0, 7.0}, {-6.0, 10.0}, {-4.0, 11.0}};
GLfloat right_collar[4][2] = {{0.0, 9.0}, {4.0, 11.0}, {6.0, 10.0}, {3.0, 7.0}};
GLfloat front_pocket[6][2] = {{5.0, 0.0}, {4.0, 1.0}, {4.0, 3.0}, {7.0, 3.0}, {7.0, 1.0}, {6.0, 0.0}};
GLfloat button1[3][2] = {{-1.0, 6.0}, {1.0, 6.0}, {0.0, 5.0}};
GLfloat button2[3][2] = {{-1.0, 3.0}, {1.0, 3.0}, {0.0, 2.0}};
GLfloat button3[3][2] = {{-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}};
GLfloat button4[3][2] = {{-1.0, -3.0}, {1.0, -3.0}, {0.0, -4.0}};

GLfloat shirt_color[9][3] = {
    {255 / 255.0f, 255 / 255.0f, 255 / 255.0f},
    {255 / 255.0f, 255 / 255.0f, 255 / 255.0f},
    {206 / 255.0f, 173 / 255.0f, 184 / 255.0f},
    {206 / 255.0f, 173 / 255.0f, 184 / 255.0f},
    {206 / 255.0f, 173 / 255.0f, 184 / 255.0f},
    {206 / 255.0f, 173 / 255.0f, 184 / 255.0f},
    {206 / 255.0f, 173 / 255.0f, 184 / 255.0f},
    {206 / 255.0f, 173 / 255.0f, 184 / 255.0f},
    {206 / 255.0f, 173 / 255.0f, 184 / 255.0f}};

GLuint VBO_shirt, VAO_shirt;
void prepare_shirt()
{
    GLsizeiptr buffer_size = sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar) + sizeof(front_pocket) + sizeof(button1) + sizeof(button2) + sizeof(button3) + sizeof(button4);

    // Initialize vertex buffer object.
    glGenBuffers(1, &VBO_shirt);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_shirt);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(left_body), left_body);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body), sizeof(right_body), right_body);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body), sizeof(left_collar), left_collar);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar), sizeof(right_collar), right_collar);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar),
                    sizeof(front_pocket), front_pocket);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar) + sizeof(front_pocket), sizeof(button1), button1);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar) + sizeof(front_pocket) + sizeof(button1), sizeof(button2), button2);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar) + sizeof(front_pocket) + sizeof(button1) + sizeof(button2), sizeof(button3), button3);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar) + sizeof(front_pocket) + sizeof(button1) + sizeof(button2) + sizeof(button3), sizeof(button4), button4);

    // Initialize vertex array object.
    glGenVertexArrays(1, &VAO_shirt);
    glBindVertexArray(VAO_shirt);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_shirt);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void draw_shirt()
{
    glBindVertexArray(VAO_shirt);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_LEFT_BODY]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_RIGHT_BODY]);
    glDrawArrays(GL_TRIANGLE_FAN, 6, 6);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_LEFT_COLLAR]);
    glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_RIGHT_COLLAR]);
    glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_FRONT_POCKET]);
    glDrawArrays(GL_TRIANGLE_FAN, 20, 6);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_BUTTON1]);
    glDrawArrays(GL_TRIANGLE_FAN, 26, 3);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_BUTTON2]);
    glDrawArrays(GL_TRIANGLE_FAN, 29, 3);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_BUTTON3]);
    glDrawArrays(GL_TRIANGLE_FAN, 32, 3);

    glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_BUTTON4]);
    glDrawArrays(GL_TRIANGLE_FAN, 35, 3);
    glBindVertexArray(0);
}

// sword

#define SWORD_BODY 0
#define SWORD_BODY2 1
#define SWORD_HEAD 2
#define SWORD_HEAD2 3
#define SWORD_IN 4
#define SWORD_DOWN 5
#define SWORD_BODY_IN 6

GLfloat sword_body[4][2] = {{-6.0, 0.0}, {-6.0, -4.0}, {6.0, -4.0}, {6.0, 0.0}};
GLfloat sword_body2[4][2] = {{-2.0, -4.0}, {-2.0, -6.0}, {2.0, -6.0}, {2.0, -4.0}};
GLfloat sword_head[4][2] = {{-2.0, 0.0}, {-2.0, 16.0}, {2.0, 16.0}, {2.0, 0.0}};
GLfloat sword_head2[3][2] = {{-2.0, 16.0}, {0.0, 19.46}, {2.0, 16.0}};
GLfloat sword_in[4][2] = {{-0.3, 0.7}, {-0.3, 15.3}, {0.3, 15.3}, {0.3, 0.7}};
GLfloat sword_down[4][2] = {{-2.0, -6.0}, {2.0, -6.0}, {4.0, -8.0}, {-4.0, -8.0}};
GLfloat sword_body_in[4][2] = {{0.0, -1.0}, {1.0, -2.732}, {0.0, -4.464}, {-1.0, -2.732}};

GLfloat sword_color[7][3] = {
    {139 / 255.0f, 69 / 255.0f, 19 / 255.0f},
    {139 / 255.0f, 69 / 255.0f, 19 / 255.0f},
    {155 / 255.0f, 155 / 255.0f, 155 / 255.0f},
    {155 / 255.0f, 155 / 255.0f, 155 / 255.0f},
    {0 / 255.0f, 0 / 255.0f, 0 / 255.0f},
    {139 / 255.0f, 69 / 255.0f, 19 / 255.0f},
    {255 / 255.0f, 0 / 255.0f, 0 / 255.0f}};

GLuint VBO_sword, VAO_sword;

void prepare_sword()
{
    GLsizeiptr buffer_size = sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in) + sizeof(sword_down) + sizeof(sword_body_in);

    // Initialize vertex buffer object.
    glGenBuffers(1, &VBO_sword);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_sword);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sword_body), sword_body);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body), sizeof(sword_body2), sword_body2);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2), sizeof(sword_head), sword_head);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head), sizeof(sword_head2), sword_head2);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2), sizeof(sword_in), sword_in);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in), sizeof(sword_down), sword_down);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in) + sizeof(sword_down), sizeof(sword_body_in), sword_body_in);

    // Initialize vertex array object.
    glGenVertexArrays(1, &VAO_sword);
    glBindVertexArray(VAO_sword);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_sword);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void draw_sword()
{
    glBindVertexArray(VAO_sword);

    glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY2]);
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

    glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_HEAD]);
    glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

    glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_HEAD2]);
    glDrawArrays(GL_TRIANGLE_FAN, 12, 3);

    glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_IN]);
    glDrawArrays(GL_TRIANGLE_FAN, 15, 4);

    glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_DOWN]);
    glDrawArrays(GL_TRIANGLE_FAN, 19, 4);

    glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY_IN]);
    glDrawArrays(GL_TRIANGLE_FAN, 23, 4);

    glBindVertexArray(0);
}

// start
typedef struct
{
    float cx, cy, rad, vx, vy;
    int big, small;
} Ball;
#define BALL 0
#define REC 1
#define TRI 2
Ball balls[2];
GLfloat ball1[360][2];
GLfloat ball2[360][2];
GLfloat rec[4][2];
GLfloat tri[3][2];

GLfloat ball_color[3][3] = {
    {0 / 255.0f, 0 / 255.0f, 0 / 255.0f},
    {255 / 255.0f, 217 / 255.0f, 28 / 255.0f},
    {222 / 255.0f, 124 / 255.0f, 137 / 255.0f}};

GLuint VBO_ball1, VAO_ball1;
GLuint VBO_ball2, VAO_ball2;

void prepare_ball1()
{
    int i;
    balls[0].cx = 0.0;
    balls[0].cy = 0.0;
    balls[0].rad = 25.0;
    balls[0].vx = 0.0;
    balls[0].vy = 0.0;
    balls[0].big = 0;
    balls[0].small = 0;
    for (i = 0; i < 360; i++)
    {
        ball1[i][0] = balls[0].cx + balls[0].rad * cos(i * TO_RADIAN);
        ball1[i][1] = balls[0].cy + balls[0].rad * sin(i * TO_RADIAN);
    }
    for (i = 0; i < 3; i = i++)
    {
        tri[i][0] = balls[0].cx + balls[0].rad * cos(120 * i * TO_RADIAN);
        tri[i][1] = balls[0].cy + balls[0].rad * sin(120 * i * TO_RADIAN);
    }

    GLsizeiptr buffer_size = sizeof(ball1) + sizeof(tri);

    glGenBuffers(1, &VBO_ball1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_ball1);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ball1), ball1);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(ball1), sizeof(tri), tri);

    glGenVertexArrays(1, &VAO_ball1);
    glBindVertexArray(VAO_ball1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_ball1);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void prepare_ball2()
{ // triangle
    int i;
    balls[1].cx = -140.0;
    balls[1].cy = 0.0;
    balls[1].rad = 40.0;
    balls[1].vx = 0.0;
    balls[1].vy = 0.0;
    for (i = 0; i < 360; i++)
    {
        ball2[i][0] = balls[1].cx + balls[1].rad * cos(i * TO_RADIAN);
        ball2[i][1] = balls[1].cy + balls[1].rad * sin(i * TO_RADIAN);
    }
    for (i = 0; i < 4; i = i++)
    {
        rec[i][0] = balls[1].cx + balls[1].rad * cos(90 * i * TO_RADIAN);
        rec[i][1] = balls[1].cy + balls[1].rad * sin(90 * i * TO_RADIAN);
    }

    GLsizeiptr buffer_size = sizeof(ball2) + sizeof(rec);

    glGenBuffers(1, &VBO_ball2);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_ball2);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ball2), ball2);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(ball2), sizeof(rec), rec);

    glGenVertexArrays(1, &VAO_ball2);
    glBindVertexArray(VAO_ball2);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_ball2);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void draw_ball1()
{
    glBindVertexArray(VAO_ball1);

    glUniform3fv(loc_primitive_color, 1, ball_color[BALL]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 360);

    glUniform3fv(loc_primitive_color, 1, ball_color[TRI]);
    glDrawArrays(GL_TRIANGLE_FAN, 360, 3);

    glBindVertexArray(0);
}

void draw_ball2()
{
    glBindVertexArray(VAO_ball2);

    glUniform3fv(loc_primitive_color, 1, ball_color[BALL]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 360);

    glUniform3fv(loc_primitive_color, 1, ball_color[REC]);
    glDrawArrays(GL_TRIANGLE_FAN, 360, 4);

    glBindVertexArray(0);
}

#define DOOR 0
#define DOOR_WINDOW 1
#define DOOR_HANDLE 2
GLfloat door[4][2] = {{-15.0, 0.0}, {-15.0, 40.0}, {15.0, 40.0}, {15.0, 0.0}};
GLfloat door_window[4][2] = {{-10.0, 25.0}, {-10.0, 35.0}, {10.0, 35.0}, {10.0, 25.0}};
GLfloat door_handle[4][2] = {{10.0, 20.0}, {12.0, 18.0}, {10.0, 16.0}, {8.0, 18.0}};
GLfloat door_color[3][3] = {
    {166 / 255.0f, 115 / 255.0f, 20 / 255.0f},
    {201 / 255.0f, 243 / 255.0f, 255 / 255.0f},
    {110 / 255.0f, 109 / 255.0f, 106 / 255.0f}};

GLuint VBO_door, VAO_door;

void prepare_door()
{
    GLsizeiptr buffer_size = sizeof(door) + sizeof(door_window) + sizeof(door_handle);

    glGenBuffers(1, &VBO_door);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_door);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(door), door);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(door), sizeof(door_window), door_window);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(door) + sizeof(door_window), sizeof(door_handle), door_handle);

    glGenVertexArrays(1, &VAO_door);
    glBindVertexArray(VAO_door);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_door);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void draw_door()
{
    glBindVertexArray(VAO_door);

    glUniform3fv(loc_primitive_color, 1, door_color[DOOR]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUniform3fv(loc_primitive_color, 1, door_color[DOOR_WINDOW]);
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

    glUniform3fv(loc_primitive_color, 1, door_color[DOOR_HANDLE]);
    glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
    glBindVertexArray(0);
}

#define OD 0
#define OD_WINDOW 1
#define OD_BACK 2

GLfloat od[4][2] = {{-15.0, 0.0}, {-15.0, 40.0}, {-40.0, 50.0}, {-40.0, 10.0}};
GLfloat od_window[4][2] = {{-20.0, 35.0}, {-20.0, 25.0}, {-35.0, 31.5}, {-35.0, 41.5}};
GLfloat od_back[4][2] = {{-15.0, 0.0}, {-15.0, 40.0}, {15.0, 40.0}, {15.0, 0.0}};
GLfloat od_color[3][3] = {
    {166 / 255.0f, 115 / 255.0f, 20 / 255.0f},
    {201 / 255.0f, 243 / 255.0f, 255 / 255.0f},
    {0 / 255.0f, 0 / 255.0f, 0 / 255.0f}};

GLuint VBO_od, VAO_od;

void prepare_opened()
{
    GLsizeiptr buffer_size = sizeof(od) + sizeof(od_window) + sizeof(od_back);

    glGenBuffers(1, &VBO_od);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_od);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(od), od);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(od), sizeof(od_window), od_window);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(od) + sizeof(od_window), sizeof(od_back), od_back);

    glGenVertexArrays(1, &VAO_od);
    glBindVertexArray(VAO_od);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_od);
    glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void draw_opened()
{
    glBindVertexArray(VAO_od);

    glUniform3fv(loc_primitive_color, 1, od_color[OD]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUniform3fv(loc_primitive_color, 1, od_color[OD_WINDOW]);
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

    glUniform3fv(loc_primitive_color, 1, od_color[OD_BACK]);
    glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
    glBindVertexArray(0);
}
// end

unsigned int timestamp = 0;
int mouseOnDoor = 0;
void display(void)
{
    int i;
    glm::mat4 ModelMatrix;

    glClear(GL_COLOR_BUFFER_BIT);

    for (i = 0; i < 5; i++)
    {
        if (big[i].exist == 1)
        {
            if ((balls[0].cx - balls[0].rad <= big[i].cx) && (big[i].cx <= balls[0].cx + balls[0].rad) && (big[i].cy <= balls[0].cy + balls[0].rad) && (balls[0].cy - balls[0].rad <= big[i].cy))
            {
                big[i].exist = 0;
                balls[0].rad *= 1.25f;
            }
        }
    }

    for (i = 0; i < 5; i++)
    {
        if (small[i].exist == 1)
        {
            if ((balls[0].cx - balls[0].rad <= small[i].cx) && (small[i].cx <= balls[0].cx + balls[0].rad) && (small[i].cy <= balls[0].cy + balls[0].rad) && (balls[0].cy - balls[0].rad <= small[i].cy))
            {
                small[i].exist = 0;
                balls[0].rad *= 0.75f;
            }
        }
    }

    int cocktail_clk = (timestamp % 1442) / 2 - 360;
    float rotation_angle_cocktail = atanf(180.0f * TO_RADIAN * cosf((cocktail_clk)*TO_RADIAN));
    float scaling_factor_cocktail = 5.0f * (1.0f - fabs(sinf(cocktail_clk * TO_RADIAN)));
    for (i = 0; i < 10; i++)
    {
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx, centery + 100 * i, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f * tanf(cocktail_clk * TO_RADIAN), (float)cocktail_clk, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f / scaling_factor_cocktail + 1.0f, 0.5f / scaling_factor_cocktail + 1.0f, 1.0f));
        ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_cocktail, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_cocktail();
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx - win_width / 4, centery + 100 * i, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f * tanf(cocktail_clk * TO_RADIAN), (float)cocktail_clk, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f / scaling_factor_cocktail + 1.0f, 0.5f / scaling_factor_cocktail + 1.0f, 1.0f));
        ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_cocktail, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_cocktail();
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx + win_width / 4, centery + 100 * i, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f * tanf(cocktail_clk * TO_RADIAN), (float)cocktail_clk, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f / scaling_factor_cocktail + 1.0f, 0.5f / scaling_factor_cocktail + 1.0f, 1.0f));
        ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_cocktail, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_cocktail();
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx, centery - 100 * i, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f * tanf(cocktail_clk * TO_RADIAN), (float)cocktail_clk, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f / scaling_factor_cocktail + 1.0f, 0.5f / scaling_factor_cocktail + 1.0f, 1.0f));
        ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_cocktail, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_cocktail();
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx - win_width / 4, centery - 100 * i, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f * tanf(cocktail_clk * TO_RADIAN), (float)cocktail_clk, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f / scaling_factor_cocktail + 1.0f, 0.5f / scaling_factor_cocktail + 1.0f, 1.0f));
        ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_cocktail, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_cocktail();
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx + win_width / 4, centery - 100 * i, 0.0f));
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f * tanf(cocktail_clk * TO_RADIAN), (float)cocktail_clk, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f / scaling_factor_cocktail + 1.0f, 0.5f / scaling_factor_cocktail + 1.0f, 1.0f));
        ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_cocktail, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_cocktail();
    }

    int bigscale = timestamp % 100;
    int bigrotate = timestamp % 100;
    if (bigrotate < 50)
    { // 0~49
        bigrotate = 360 * (pow(bigrotate, 4)) / 5764801;
    }
    else
    {
        bigrotate = -360 * (pow(bigrotate - 100, 4)) / 5764801;
    }
    for (i = 0; i < 5; i++)
    {
        if (big[i].exist == 1)
        {
            ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(big[i].cx, big[i].cy, 0.0f));
            ModelMatrix = glm::rotate(ModelMatrix, (float)bigrotate * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(pow((0.75 + (float)bigscale / 400), 4), pow((0.75 + (float)bigscale / 400), 4), 1.0f));
            ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
            glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
            draw_airplane();
        }
    }

    int smallrotate = timestamp % 180;
    if (smallrotate < 45)
        ;
    else if (smallrotate < 90)
    {
        smallrotate = -smallrotate + 90;
    }
    else if (smallrotate < 135)
    {
        smallrotate = smallrotate - 90;
    }
    else
    {
        smallrotate = -smallrotate + 180;
    }

    for (i = 0; i < 5; i++)
    {
        if (small[i].exist == 1)
        {
            ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(small[i].cx, small[i].cy, 0.0f));
            ModelMatrix = glm::rotate(ModelMatrix, (float)(smallrotate - 22.5f) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.0f + ((float)smallrotate / 45 + 0.2f), 1.0f + ((float)smallrotate / 45 + 0.2f), 1.0f));
            ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
            glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
            draw_cake();
        }
    }

    if (balls[0].vx >= 0.025 || balls[0].vx <= -0.025 || balls[0].vy >= 0.025 || balls[0].vy <= -0.025)
    {
        balls[0].cx += balls[0].vx;
        balls[0].cy += balls[0].vy;
        if (balls[0].cx - balls[0].rad <= -win_width / 2)
        {
            balls[0].cx = -win_width / 2 + balls[0].rad + (-win_width / 2 + balls[0].rad - balls[0].cx);
            balls[0].vx = -balls[0].vx;
        }
        if (balls[0].cx + balls[0].rad >= win_width / 2)
        {
            balls[0].cx = (win_width / 2 - balls[0].rad) - (balls[0].cx - win_width / 2 + balls[0].rad);
            balls[0].vx = -balls[0].vx;
        }
        if (balls[0].cy - balls[0].rad <= -win_height / 2)
        {
            balls[0].cy = -win_height / 2 + balls[0].rad + (-win_height / 2 + balls[0].rad - balls[0].cy);
            balls[0].vy = -balls[0].vy;
        }
        if (balls[0].cy + balls[0].rad >= win_height / 2)
        {
            balls[0].cy = (win_height / 2 - balls[0].rad) - (balls[0].cy - win_height / 2 + balls[0].rad);
            balls[0].vy = -balls[0].vy;
        }
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx, balls[0].cy, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, balls[0].vx * balls[0].vy * 10 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(balls[0].rad / 25.0f, balls[0].rad / 25.0f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_ball1();
        balls[0].vx = balls[0].vx / (1 + (balls[0].rad / 2000) * 1.0000025);
        balls[0].vy = balls[0].vy / (1 + (balls[0].rad / 2000) * 1.0000025);
    }
    else
    {
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx, balls[0].cy, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(balls[0].rad / 25.0f, balls[0].rad / 25.0f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_ball1();
    }

    float length, angle;
    if (clickedOnBall1)
    {
        length = sqrt(pow((dxo - balls[0].cx), 2) + pow((dyo - balls[0].cy), 2));
        angle = atan2(dxo - balls[0].cx, dyo - balls[0].cy);

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx - 80, balls[0].cy, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.5f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_shirt(); // 1

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx - 55, balls[0].cy + 55, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, -45.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.5f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_shirt(); // 2

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx, balls[0].cy + 80, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.5f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_shirt(); // 3

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx + 55, balls[0].cy + 55, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, -135.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.5f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_shirt(); // 4

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx + 80, balls[0].cy, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, -180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.5f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_shirt(); // 5

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx + 55, balls[0].cy - 55, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, -225.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.5f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_shirt(); // 6

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx, balls[0].cy - 80, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, -270.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.5f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_shirt(); // 7

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx - 55, balls[0].cy - 55, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, -315.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 3.5f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_shirt(); // 8

        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(balls[0].cx, balls[0].cy, 0.0f));
        ModelMatrix = glm::rotate(ModelMatrix, -angle, glm::vec3(0.0f, 0.0f, 1.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.0f, -length / 25, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_sword();
    }

    ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(win_width / 2 - 60.0f, win_height / 2 - 80.0f, 0.0f));
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.2f, 1.2f, 1.0f));
    ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_door();
    if (mouseOnDoor == 1)
    { // mouseOnDoor
        ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(win_width / 2 - 60.0f, win_height / 2 - 80.0f, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.2f, 1.2f, 1.0f));
        ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
        glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
        draw_opened();
    }

    glFlush();
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27:                 // ESC key
        glutLeaveMainLoop(); // Incur destuction callback for cleanups.
        break;
    }
}

void mouse(int button, int state, int x, int y)
{
    int xo = x - win_width / 2;
    int yo = win_height / 2 - y;
    int xlength, ylength;
    int i;
    if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN))
    {
        if ((win_width / 2 - 82 <= xo) && (xo <= win_width / 2 - 40) && (28 <= y) && (y <= 82))
        {
            for (i = 0; small[i].exist == 1; i++)
                ;
            if (i != 5)
            {
                small[i].cx = (float)((rand() % (win_width / 2)) - win_width / 4);
                small[i].cy = (float)((rand() % (win_height / 2)) - win_height / 4);
                small[i].exist = 1;
            }
        }
    }
    if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
    {
        leftbuttonpressed = 1;
        if ((win_width / 2 - 82 <= xo) && (xo <= win_width / 2 - 40) && (28 <= y) && (y <= 82))
        {
            if (mouseOnDoor == 0)
            {
                mouseOnDoor = 1;
                for (i = 0; big[i].exist == 1; i++)
                    ;
                if (i != 5)
                {
                    big[i].cx = (float)((rand() % (win_width / 2)) - win_width / 4);
                    big[i].cy = (float)((rand() % (win_height / 2)) - win_height / 4);
                    big[i].exist = 1;
                }
            }
            else if (mouseOnDoor == 1)
            {
                glutLeaveMainLoop();
                return;
            }
        }
        else if (mouseOnDoor == 1 && !((win_width / 2 - 82 <= xo) && (xo <= win_width / 2 - 40) && (28 <= y) && (y <= 82)))
        {
            mouseOnDoor = 0;
        }

        if ((balls[0].cx - balls[0].rad <= xo) && (xo <= balls[0].cx + balls[0].rad) && (balls[0].cy - balls[0].rad <= yo) && (yo <= balls[0].cy + balls[0].rad))
        { // ball1 click
            clickedOnBall1 = 1;
            prevx = xo;
            prevy = yo;
            mprevx = xo;
            mprevy = yo;
        }

        if ((balls[1].cx - balls[1].rad <= xo) && (xo <= balls[1].cx + balls[1].rad) && (balls[1].cy - balls[1].rad <= yo) && (yo <= balls[1].cy + balls[1].rad))
        {
            clickedOnBall2 = 1;
            prevx = xo;
            prevy = yo;
            mprevx = xo;
            mprevy = yo;
        }
    }
    else if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP))
    {
        leftbuttonpressed = 0;

        if (clickedOnBall1)
        {
            balls[0].vx = xo - balls[0].cx;

            balls[0].vy = yo - balls[0].cy;

            if (balls[0].vx > 160)
                balls[0].vx = 160;
            if (balls[0].vy > 160)
                balls[0].vy = 160;
            if (balls[0].vx < -160)
                balls[0].vx = -160;
            if (balls[0].vy < -160)
                balls[0].vy = -160;
            balls[0].vx /= 10;
            balls[0].vx = -balls[0].vx;
            balls[0].vy /= 10;
            balls[0].vy = -balls[0].vy;
        }
        else if (clickedOnBall2)
        {
            balls[1].vx = xo - balls[1].cx;
            if (balls[1].vx > 160)
                balls[1].vx = 160;
            balls[1].vy = yo - balls[1].cy;
            if (balls[1].vy > 160)
                balls[1].vy = 160;
        }
        clickedOnBall1 = 0;
        clickedOnBall2 = 0;
    }
    glutPostRedisplay();
}

void motion(int x, int y)
{
    /*static int delay = 0;
    static float tmpx = 0.0, tmpy = 0.0;
    float dx, dy;
    if (leftbuttonpressed) {
        centerx =  x - win_width/2.0f, centery = (win_height - y) - win_height/2.0f;
        if (delay == 8) {
            dx = centerx - tmpx;
            dy = centery - tmpy;

            if (dx > 0.0) {
                rotate_angle = atan(dy / dx) + 90.0f*TO_RADIAN;
            }
            else if (dx < 0.0) {
                rotate_angle = atan(dy / dx) - 90.0f*TO_RADIAN;
            }
            else if (dx == 0.0) {
                if (dy > 0.0) rotate_angle = 180.0f*TO_RADIAN;
                else  rotate_angle = 0.0f;
            }
            tmpx = centerx, tmpy = centery;
            delay = 0;
        }
        glutPostRedisplay();
        delay++;
    }*/
    dxo = x - win_width / 2;
    dyo = win_height / 2 - y;
}

void reshape(int width, int height)
{
    win_width = width, win_height = height;

    glViewport(0, 0, win_width, win_height);
    ProjectionMatrix = glm::ortho(-win_width / 2.0, win_width / 2.0,
                                  -win_height / 2.0, win_height / 2.0, -1000.0, 1000.0);
    ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

    // update_axes();
    // update_line();

    glutPostRedisplay();
}

void cleanup(void)
{
    /*glDeleteVertexArrays(1, &VAO_axes);
    glDeleteBuffers(1, &VBO_axes);

    glDeleteVertexArrays(1, &VAO_line);
    glDeleteBuffers(1, &VBO_line);

    glDeleteVertexArrays(1, &VAO_airplane);
    glDeleteBuffers(1, &VBO_airplane);*/

    glDeleteVertexArrays(1, &VAO_door);
    glDeleteBuffers(1, &VBO_door);

    glDeleteVertexArrays(1, &VAO_od);
    glDeleteBuffers(1, &VBO_od);

    glDeleteVertexArrays(1, &VAO_ball1);
    glDeleteBuffers(1, &VBO_ball1);

    glDeleteVertexArrays(1, &VAO_ball2);
    glDeleteBuffers(1, &VBO_ball2);

    glDeleteVertexArrays(1, &VAO_cocktail);
    glDeleteBuffers(1, &VBO_cocktail);

    glDeleteVertexArrays(1, &VAO_shirt);
    glDeleteBuffers(1, &VBO_shirt);

    glDeleteVertexArrays(1, &VAO_sword);
    glDeleteBuffers(1, &VBO_sword);
}

void timer(int value)
{
    timestamp = (timestamp + 1) % UINT_MAX;
    glutPostRedisplay();
    glutTimerFunc(5, timer, 0);
}

void register_callbacks(void)
{
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);
    glutCloseFunc(cleanup);
    glutTimerFunc(5, timer, 0);
}

void prepare_shader_program(void)
{
    ShaderInfo shader_info[3] = {
        {GL_VERTEX_SHADER, "Shaders/simple.vert"},
        {GL_FRAGMENT_SHADER, "Shaders/simple.frag"},
        {GL_NONE, NULL}};

    h_ShaderProgram = LoadShaders(shader_info);
    glUseProgram(h_ShaderProgram);

    loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProjectionMatrix");
    loc_primitive_color = glGetUniformLocation(h_ShaderProgram, "u_primitive_color");
}

void initialize_OpenGL(void)
{
    glEnable(GL_MULTISAMPLE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(61 / 255.0f, 135 / 255.0f, 89 / 255.0f, 1.0f);
    ViewMatrix = glm::mat4(1.0f);
}

void prepare_scene(void)
{
    // prepare_axes();
    // prepare_line();
    // prepare_airplane();
    prepare_door();
    prepare_opened();
    prepare_ball1();
    prepare_ball2();
    prepare_cocktail();
    prepare_shirt();
    prepare_sword();
    prepare_airplane();
    prepare_cake();
}

void initialize_renderer(void)
{
    register_callbacks();
    prepare_shader_program();
    initialize_OpenGL();
    prepare_scene();
}

void initialize_glew(void)
{
    GLenum error;

    glewExperimental = GL_TRUE;

    error = glewInit();
    if (error != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
        exit(-1);
    }
    fprintf(stdout, "*********************************************************\n");
    fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
    fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
    fprintf(stdout, "*********************************************************\n\n");
}

void greetings(char *program_name, char messages[][256], int n_message_lines)
{
    fprintf(stdout, "**************************************************************\n\n");
    fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
    fprintf(stdout, "    This program was coded for CSE4170 students\n");
    fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

    for (int i = 0; i < n_message_lines; i++)
        fprintf(stdout, "%s\n", messages[i]);
    fprintf(stdout, "\n**************************************************************\n\n");

    initialize_glew();
}

#define N_MESSAGE_LINES 2
void main(int argc, char *argv[])
{
    char program_name[64] = "Sogang CSE4170 Simple2DTransformation_GLSL_3.0";
    char messages[N_MESSAGE_LINES][256] = {
        "    - Mouse used: L-click and move",
    };

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize(700, 800);
    glutInitContextVersion(4, 0);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutCreateWindow(program_name);

    greetings(program_name, messages, N_MESSAGE_LINES);
    initialize_renderer();

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();
}
