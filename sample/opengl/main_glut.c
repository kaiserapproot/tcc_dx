/*
 * main_glut.c - macOS port of main_gl.c (rotating cube + axes).
 *
 * main_gl.c opens its window with Win32 + WGL, which macOS does not have.
 * Here GLUT (GLUT.framework, part of the macOS SDK) creates the window and
 * the GL context; the drawing code is the same as main_gl.c.
 *
 * Build: sh build_opengl_mac.sh   (see that script for the tcc command line)
 *
 * Keys:  ESC quit / Up, Down rotation speed / Space full screen on, off
 *        right click: menu (stands in for the Win32 menu of main_gl.c)
 * Option: -frames N  quit after N frames (used by the automated check)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GLUT/glut.h>

const int width = 800;
const int height = 600;

float f = 0.4;
int full_screen = 0;
int max_frames = 0;    /* 0 = run until closed */
int frames = 0;

enum { MENU_NOT_IMPLEMENTED = 1, MENU_ABOUT, MENU_EXIT };

float colors[6][4][3] =
{
0.0, 1.0, 1.0,
0.0, 1.0, 1.0,
0.0, 1.0, 1.0,
0.0, 1.0, 1.0,
1.0, 1.0, 0.0,
1.0, 1.0, 0.0,
1.0, 1.0, 0.0,
1.0, 1.0, 0.0,
1.0, 0.0, 1.0,
1.0, 0.0, 1.0,
1.0, 0.0, 1.0,
1.0, 0.0, 1.0,
0.0, 0.0, 1.0,
0.0, 0.0, 1.0,
0.0, 0.0, 1.0,
0.0, 0.0, 1.0,
0.0, 1.0, 0.0,
0.0, 1.0, 0.0,
0.0, 1.0, 0.0,
0.0, 1.0, 0.0,
1.0, 0.0, 0.0,
1.0, 0.0, 0.0,
1.0, 0.0, 0.0,
1.0, 0.0, 0.0,
};

float cube[6][4][3] =
{
-1.0, -1.0, -1.0,
-1.0, -1.0, 1.0,
-1.0, 1.0, 1.0,
-1.0, 1.0, -1.0,

-1.0, -1.0, -1.0,
-1.0, 1.0, -1.0,
1.0, 1.0, -1.0,
1.0, -1.0, -1.0,

-1.0, -1.0, -1.0,
1.0, -1.0, -1.0,
1.0, -1.0, 1.0,
-1.0, -1.0, 1.0,

-1.0, -1.0, 1.0,
1.0, -1.0, 1.0,
1.0, 1.0, 1.0,
-1.0, 1.0, 1.0,

-1.0, 1.0, -1.0,
-1.0, 1.0, 1.0,
1.0, 1.0, 1.0,
1.0, 1.0, -1.0,

1.0, -1.0, -1.0,
1.0, 1.0, -1.0,
1.0, 1.0, 1.0,
1.0, -1.0, 1.0,
};

void drawAxes()
{
    glLineWidth(5);
    glBegin(GL_LINES);
    glColor3f(0.6, 0.0, 0.0); glVertex3f(0.0, 0.0, 0.0); glVertex3f(3.0, 0.0, 0.0);
    glColor3f(0.0, 0.6, 0.0); glVertex3f(0.0, 0.0, 0.0); glVertex3f(0.0, 3.0, 0.0);
    glColor3f(0.0, 0.0, 0.6); glVertex3f(0.0, 0.0, 0.0); glVertex3f(0.0, 0.0, 3.0);
    glEnd();
}

void drawCube()
{
    int i, j;
    for (i = 0; i < 6; i++) {
        glBegin(GL_QUADS);
        for (j = 0; j < 4; j++) {
            glColor3f(colors[i][j][0], colors[i][j][1], colors[i][j][2]);
            glVertex3f(cube[i][j][0], cube[i][j][1], cube[i][j][2]);
        }
        glEnd();
    }
}

/* the projection is reset on every resize; the rotation accumulated in the
   modelview matrix is kept, as in main_gl.c */
void changeSize(int w, int h)
{
    float ratio;
    if (h == 0)
        h = 1;
    ratio = (float)w / (float)h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void Init()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    changeSize(width, height);
    glTranslatef(0.0, 0.0, -6.0);
    glRotatef(30.0, 1.0, 1.0, 1.0);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawAxes();
    drawCube();
    glRotatef(f, 1.0, 1.0, 1.0);
    glutSwapBuffers();
    if (max_frames && ++frames >= max_frames) {
        printf("GL_VERSION: %s\n", (const char *)glGetString(GL_VERSION));
        printf("GL_RENDERER: %s\n", (const char *)glGetString(GL_RENDERER));
        printf("frames: %d\n", frames);
        exit(glGetError() == GL_NO_ERROR ? 0 : 1);
    }
}

/* main_gl.c redraws from its message loop with Sleep(1); ~60 fps here */
void timer(int value)
{
    (void)value;
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void toggleFullScreen()
{
    if (!full_screen) {
        glutFullScreen();
        full_screen = 1;
    } else {
        glutReshapeWindow(width, height);
        glutPositionWindow(0, 0);
        full_screen = 0;
    }
}

void keyboard(unsigned char key, int x, int y)
{
    (void)x, (void)y;
    switch (key) {
    case 27:            /* ESC */
        exit(0);
    case ' ':
        toggleFullScreen();
        break;
    }
}

void special(int key, int x, int y)
{
    (void)x, (void)y;
    switch (key) {
    case GLUT_KEY_UP:
        f += 0.1;
        break;
    case GLUT_KEY_DOWN:
        f -= 0.1;
        break;
    }
}

void menu(int item)
{
    switch (item) {
    case MENU_NOT_IMPLEMENTED:
        printf("Function Not Yet Implemented.\n");
        break;
    case MENU_ABOUT:
        printf("About... macOS (GLUT) example version 0.01\n");
        break;
    case MENU_EXIT:
        exit(0);
    }
}

int main(int argc, char **argv)
{
    int i;

    glutInit(&argc, argv);
    for (i = 1; i < argc; i++)
        if (strcmp(argv[i], "-frames") == 0 && i + 1 < argc)
            max_frames = atoi(argv[++i]);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(width, height);
    glutCreateWindow("OpenGL Sample");

    glutCreateMenu(menu);
    glutAddMenuEntry("New / Open / Save (not implemented)", MENU_NOT_IMPLEMENTED);
    glutAddMenuEntry("About...", MENU_ABOUT);
    glutAddMenuEntry("Exit", MENU_EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutDisplayFunc(display);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(16, timer, 0);

    Init();
    glutMainLoop();
    return 0;
}
