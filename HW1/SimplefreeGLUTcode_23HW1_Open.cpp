#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#define TO_RADIAN 0.017453292519943296

int rightbuttonpressed = 0;
int leftbuttonpressed = 0, center_selected = 0;

float r, g, b;        // Background color
float px, py, qx, qy; // Line (px, py) --> (qx, qy)
float xlen, ylen, centx, centy;
int n_object_points = 6;
int n_object2_points = 4;
float object[6][2], object_center_x, object_center_y;
float object2[4][2], object2_center_x, object2_center_y;
float object2prev[4][2];
float rotation_angle_in_degree;
int window_width, window_height;

void draw_axes()
{
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(1.0f, 0.0f);
    glVertex2f(0.975f, 0.025f);
    glVertex2f(1.0f, 0.0f);
    glVertex2f(0.975f, -0.025f);
    glVertex2f(1.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 1.0f);
    glVertex2f(0.025f, 0.975f);
    glVertex2f(0.0f, 1.0f);
    glVertex2f(-0.025f, 0.975f);
    glVertex2f(0.0f, 1.0f);
    glEnd();
    glLineWidth(1.0f);

    glPointSize(7.0f);
    glBegin(GL_POINTS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glEnd();
    glPointSize(1.0f);
}
void draw_line(float px, float py, float qx, float qy)
{
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(px, py);
    glVertex2f(qx, qy);
    glEnd();
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2f(px, py);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(qx, qy);
    glEnd();
    glPointSize(1.0f);
}

void draw_object(void)
{
    glBegin(GL_LINE_LOOP);
    glColor3f(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < 6; i++)
        glVertex2f(object[i][0], object[i][1]);
    glEnd();
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 6; i++)
        glVertex2f(object[i][0], object[i][1]);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2f(object_center_x, object_center_y);
    glEnd();
    glPointSize(1.0f);
}

void draw_object2(void)
{
    glBegin(GL_LINE_LOOP);
    glColor3f(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < 4; i++)
        glVertex2f(object2[i][0], object2[i][1]);
    glEnd();
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 4; i++)
        glVertex2f(object2[i][0], object2[i][1]);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2f(object2_center_x, object2_center_y);
    glEnd();
    glPointSize(1.0f);
}

void display(void)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    draw_axes();
    draw_line(px, py, qx, qy);
    draw_object();
    draw_object2();
    glFlush();
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'r':
        r = 1.0f;
        g = b = 0.0f;
        fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
        glutPostRedisplay();
        break;
    case 'g':
        g = 1.0f;
        r = b = 0.0f;
        fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
        glutPostRedisplay();
        break;
    case 'b':
        b = 1.0f;
        r = g = 0.0f;
        fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
        glutPostRedisplay();
        break;
    case 's':
        r = 250.0f / 255.0f, g = 128.0f / 255.0f, b = 114.0f / 255.0f;
        fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
        glutPostRedisplay();
        break;
    case 'q':
        glutLeaveMainLoop();
        break;
    }
}

void special(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_LEFT:
        r -= 0.1f;
        if (r < 0.0f)
            r = 0.0f;
        fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
        glutPostRedisplay();
        break;
    case GLUT_KEY_RIGHT:
        r += 0.1f;
        if (r > 1.0f)
            r = 1.0f;
        fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
        glutPostRedisplay();
        break;
    case GLUT_KEY_DOWN:
        g -= 0.1f;
        if (g < 0.0f)
            g = 0.0f;
        fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
        glutPostRedisplay();
        break;
    case GLUT_KEY_UP:
        g += 0.1f;
        if (g > 1.0f)
            g = 1.0f;
        fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
        glutPostRedisplay();
        break;
    }
}

int prevx, prevy;
int specialKey;
int pressedOnVertex = 0;
void mousepress(int button, int state, int x, int y)
{
    int i;
    static int clickcnt = 0;
    specialKey = glutGetModifiers();
    float tempx, tempy;
    tempx = (x - window_width / 2.0f) / 250;
    tempy = (window_height / 2.0f - y) / 250;
    if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
    {
        leftbuttonpressed = 1;
        prevx = x;
        prevy = y;
        if (((px - 0.02f <= tempx) && (tempx <= px + 0.02f)) && ((py - 0.02f <= tempy) && (tempy <= py + 0.02f)))
        {
            pressedOnVertex = 1;
        }
        if (((object2_center_x - 0.02f <= tempx) && (tempx <= object2_center_x + 0.02f)) && ((object2_center_y - 0.02f <= tempy) && (tempy <= object2_center_y + 0.02f)))
        {
            for (i = 0; i < n_object2_points; i++)
            {
                object2[i][0] = 0.25f * object2[i][1] - 0.25f * object2_center_y + object2[i][0];
            }
            r += 0.01f;
            clickcnt++;
        }
    }
    else if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP))
    {
        leftbuttonpressed = 0;
        pressedOnVertex = 0;
    }
    else if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN))
    {
        rightbuttonpressed = 1;
        prevx = x;
        prevy = y;
        if (((object2_center_x - 0.02f <= tempx) && (tempx <= object2_center_x + 0.02f)) && ((object2_center_y - 0.02f <= tempy) && (tempy <= object2_center_y + 0.02f)))
        {
            for (i = 0; i < n_object2_points; i++)
            {
                object2[i][1] = 0.25f * object2[i][0] - 0.25f * object2_center_x + object2[i][1];
            }
            g += 0.01f;
            clickcnt++;
        }
    }
    else if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_UP))
    {
        rightbuttonpressed = 0;
    }

    if (clickcnt % 15 == 14)
    {
        object2[0][0] = object2prev[0][0];
        object2[0][1] = object2prev[0][1];
        object2[1][0] = object2prev[1][0];
        object2[1][1] = object2prev[1][1];
        object2[2][0] = object2prev[2][0];
        object2[2][1] = object2prev[2][1];
        object2[3][0] = object2prev[3][0];
        object2[3][1] = object2prev[3][1];

        for (i = 0; i < n_object2_points; i++)
        {
            object2[i][0] = 1.5f * object2[i][0] - 1.5f * object2_center_x + object2_center_x;
            object2[i][1] = 1.5f * object2[i][1] - 1.5f * object2_center_y + object2_center_y;
        }

        object2prev[0][0] = object2[0][0];
        object2prev[0][1] = object2[0][1];
        object2prev[1][0] = object2[1][0];
        object2prev[1][1] = object2[1][1];
        object2prev[2][0] = object2[2][0];
        object2prev[2][1] = object2[2][1];
        object2prev[3][0] = object2[3][0];
        object2prev[3][1] = object2[3][1];

        r = 250.0f / 255.0f;
        g = 128.0f / 255.0f;
        b += 0.1f;
        b = (b >= 0.75f) ? 114.0f / 255.0f : b;
    }
    glutPostRedisplay();
}

void mousemove(int x, int y)
{
    specialKey = glutGetModifiers();
    int i;
    int dx, dy;
    dx = x - prevx;
    dy = prevy - y;
    float tempx, tempy;
    tempx = (float)dx / 250;
    tempy = (float)dy / 250;
    if (rightbuttonpressed && specialKey == GLUT_ACTIVE_ALT)
    {
        for (i = 0; i < n_object_points; i++)
        {
            object[i][0] += tempx;
            object[i][1] += tempy;
        }
        object_center_x += tempx;
        object_center_y += tempy;
    }
    else if (rightbuttonpressed && specialKey == GLUT_ACTIVE_CTRL)
    {
        for (i = 0; i < n_object_points; i++)
        {
            object[i][0] = pow(2.5f, tempx) * object[i][0] - pow(2.5f, tempx) * object_center_x + object_center_x;
            object[i][1] = pow(2.5f, tempx) * object[i][1] - pow(2.5f, tempx) * object_center_y + object_center_y;
        }
    }
    else if (pressedOnVertex)
    {
        if (specialKey == GLUT_ACTIVE_SHIFT)
        {
            px += tempx;
            py += tempy;
        }
    }
    prevx = x;
    prevy = y;
    glutPostRedisplay();
}

void reshape(int width, int height)
{
    // DO NOT MODIFY THIS FUNCTION!!!
    window_width = width, window_height = height;
    glViewport(0.0f, 0.0f, window_width, window_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-window_width / 500.0f, window_width / 500.0f, -window_height / 500.0f, window_height / 500.0f, -1.0f, 1.0f);

    glutPostRedisplay();
}

void wheelmove(int wheel, int direction, int x, int y)
{
    if (direction < 0)
    { // up
        qx = qx * cos(rotation_angle_in_degree * TO_RADIAN) - px * cos(rotation_angle_in_degree * TO_RADIAN) + px + qy * sin(rotation_angle_in_degree * TO_RADIAN) - py * sin(rotation_angle_in_degree * TO_RADIAN);
        qy = -qx * sin(rotation_angle_in_degree * TO_RADIAN) + px * sin(rotation_angle_in_degree * TO_RADIAN) + qy * cos(rotation_angle_in_degree * TO_RADIAN) - py * cos(rotation_angle_in_degree * TO_RADIAN) + py;
    }
    else
    { // down
        qx = qx * cos(-rotation_angle_in_degree * TO_RADIAN) - px * cos(-rotation_angle_in_degree * TO_RADIAN) + px + qy * sin(-rotation_angle_in_degree * TO_RADIAN) - py * sin(-rotation_angle_in_degree * TO_RADIAN);
        qy = -qx * sin(-rotation_angle_in_degree * TO_RADIAN) + px * sin(-rotation_angle_in_degree * TO_RADIAN) + qy * cos(-rotation_angle_in_degree * TO_RADIAN) - py * cos(-rotation_angle_in_degree * TO_RADIAN) + py;
    }
    glutPostRedisplay();
}

void close(void)
{
    fprintf(stdout, "\n^^^ The control is at the close callback function now.\n\n");
}

void register_callbacks(void)
{
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mousepress);
    glutMotionFunc(mousemove);
    glutReshapeFunc(reshape);
    glutCloseFunc(close);
    glutMouseWheelFunc(wheelmove);
}

void initialize_renderer(void)
{
    register_callbacks();
    r = 250.0f / 255.0f, g = 128.0f / 255.0f, b = 114.0f / 255.0f; // Background color = Salmon
    px = -0.95f, py = 0.750f, qx = 0.65f, qy = 0.15f;
    rotation_angle_in_degree = 1.0f; // 1 degree

    float sq_cx = 0.35f, sq_cy = -0.25f, sq_side = 0.25f;
    object[0][0] = sq_cx + sq_side;
    object[0][1] = sq_cy + sq_side;
    object[1][0] = sq_cx + 2.5 * sq_side;
    object[1][1] = sq_cy + 2.5 * sq_side;
    object[2][0] = sq_cx - sq_side;
    object[2][1] = sq_cy + sq_side;
    object[3][0] = sq_cx - sq_side;
    object[3][1] = sq_cy - sq_side;
    object[4][0] = sq_cx - 2.0 * sq_side;
    object[4][1] = sq_cy - 1.5 * sq_side;
    object[5][0] = sq_cx + sq_side;
    object[5][1] = sq_cy - sq_side;
    object_center_x = object_center_y = 0.0f;
    for (int i = 0; i < n_object_points; i++)
    {
        object_center_x += object[i][0];
        object_center_y += object[i][1];
    }
    object_center_x /= n_object_points;
    object_center_y /= n_object_points;

    xlen = 0.1f, ylen = 0.1f, centx = -0.8f, centy = -1.0f;
    object2[0][0] = xlen + centx;
    object2[0][1] = ylen + centy;
    object2[1][0] = -xlen + centx;
    object2[1][1] = ylen + centy;
    object2[2][0] = -xlen + centx;
    object2[2][1] = -ylen + centy;
    object2[3][0] = xlen + centx;
    object2[3][1] = -ylen + centy;
    object2_center_x = object2_center_y = 0.0f;
    for (int i = 0; i < n_object2_points; i++)
    {
        object2_center_x += object2[i][0];
        object2_center_y += object2[i][1];
    }
    object2_center_x /= n_object2_points;
    object2_center_y /= n_object2_points;

    object2prev[0][0] = object2[0][0];
    object2prev[0][1] = object2[0][1];
    object2prev[1][0] = object2[1][0];
    object2prev[1][1] = object2[1][1];
    object2prev[2][0] = object2[2][0];
    object2prev[2][1] = object2[2][1];
    object2prev[3][0] = object2[3][0];
    object2prev[3][1] = object2[3][1];
}

void initialize_glew(void)
{
    GLenum error;

    glewExperimental = TRUE;
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
    fprintf(stdout, "    This program was coded for CSE4170 students, 20211584 Junyeong JANG\n");
    fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

    for (int i = 0; i < n_message_lines; i++)
        fprintf(stdout, "%s\n", messages[i]);
    fprintf(stdout, "\n**************************************************************\n\n");

    initialize_glew();
}

#define N_MESSAGE_LINES 4
void main(int argc, char *argv[])
{
    char program_name[64] = "Sogang CSE4170 Simple 2D Transformations by 20211584";
    char messages[N_MESSAGE_LINES][256] = {
        "    - Keys used: 'r', 'g', 'b', 's', 'q'",
        "    - Special keys used: LEFT, RIGHT, UP, DOWN",
        "    - Mouse used: SHIFT/L-click and move, ALT/R-click and move, CTRL/R-click and move",
        "    - Wheel used: up and down scroll"
        "    - Other operations: window size change"};

    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE); // <-- Be sure to use this profile for this example code!
    //	glutInitContextProfile(GLUT_CORE_PROFILE);

    glutInitDisplayMode(GLUT_RGBA);

    glutInitWindowSize(750, 750);
    glutInitWindowPosition(500, 200);
    glutCreateWindow(program_name);

    greetings(program_name, messages, N_MESSAGE_LINES);
    initialize_renderer();

    // glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_EXIT); // default
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    glutMainLoop();
    fprintf(stdout, "^^^ The control is at the end of main function now.\n\n");
}