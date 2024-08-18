/*
 * Semestral work B4M39MMA - Simulace zvlnění vodní hladiny (water ripple simulation)
 * Giang Chau Nguenová (nguyegia@fel.cvut.cz)
 * Winter semester 2018
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <GL/freeglut.h>

//source: https://github.com/nothings/stb/blob/master/stb_image.h
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN_WIDTH 640
#define WIN_HEIGHT 360
#define WATERSIZE_X 270
#define WATERSIZE_Y 480
#define SHADING 0.01f
#define REFRACTION 1.5f
#define DAMPING 0.03f // 0.07f
#define RIPPLE 200
#define MAX_DELAY 60 // 15
#define BLEND_FACTOR 0.1f
#define TEXTUREDIR "../textures/blue_tiles.jpg"

int repeat_u = 1;
int repeat_v = 1;
float water[2][WATERSIZE_Y][WATERSIZE_X];
int buff_1 = 0, buff_2 = 1;
int num  = 0;
int delay = 15; // 30
int rain = 1;
int frame = 0;
int timebase = 0;
float currWidth = 0;
float currHeight = 0;

GLuint textureID;
int spin_x, spin_y, spin_z; /* x-y rotation and zoom */
int old_x, old_y, move_z;

/**
 * The function updates height field based on current and previous values saved in the array "water".
 * Algorithm from: https://web.archive.org/web/20160505235423/http://freespace.virgin.net/hugo.elias/graphics/x_water.htm
 */
void calcWater() {
	int x, y;
	float val;
	for(y = 1; y < WATERSIZE_Y-1; ++y) { 	//non-edge elements
		for(x = 1; x < WATERSIZE_X-1; x++) {
            val = ( water[buff_1][y][x-1] +
                    water[buff_1][y][x+1] +
                    water[buff_1][y-1][x] +
                    water[buff_1][y+1][x] )/2.0f - water[buff_2][y][x];
            val = val - (val * DAMPING);
            water[buff_2][y][x] = val;
		}
	}
}

/**
 * Window size changes
 * @param width of window
 * @param height of window
 */
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
    gluPerspective(45.0, (GLfloat)width/(GLfloat)height, 1.0, 1500.0);
}

/**
 * Loads texture from dir TEXTUREDIR.
 * @return Return number of the texture unit
 */
GLuint loadTexture(){
    GLuint texture;
    int width, height, nrChannels;
    unsigned char *img = stbi_load(TEXTUREDIR, &width, &height, &nrChannels, 0);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    if (img != NULL){
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    } else{
        printf("Texture error\n");
    }
    stbi_image_free(img);
    return texture;
}

/**
 * Set water array with 0 values.
 */
void resetWaterGrid(){
    int i,j;
    for( i = 0; i < WATERSIZE_Y; ++i){
        for( j = 0; j < WATERSIZE_X; ++j) {
            water[0][i][j] = 0;
            water[1][i][j] = 0;
        }
    }
}

/**
 * Set a value in the water array at [x, y] coordinations. 
 * @param x index x in the "water" array
 * @param y index y in the "water" array
 * @param val new height value
 */
void ripple(int x, int y, int val){
    int valc = (val - 75) > 0 ? (val-75): 0;
    water[buff_1][y][x] = val;
    water[buff_1][y][x+1] = valc;
    water[buff_1][y+1][x] = valc;
    water[buff_1][y-1][x] = valc;
    water[buff_1][y][x-1] = valc;
    water[buff_1][y+1][x+1] = valc;
    water[buff_1][y-1][x-1] = valc;
    water[buff_1][y-1][x+1] = valc;
    water[buff_1][y+1][x-1] = valc;
}

/**
 * Called in block glBegin-glEnd, inserts vertices with the texture coordinates. The texture is mapped to imitate refraction.
 * @param i index i in the "water" array
 * @param j index j in the "water" array
 */
void drawVert(int i, int j){
    float xOffset, yOffset, u, v, shading;
    xOffset = water[buff_1][i][j-1] - water[buff_1][i][j+1];
    yOffset = water[buff_1][i-1][j] - water[buff_1][i+1][j];
    u = ((i-1+yOffset*REFRACTION)/(WATERSIZE_Y-3))*repeat_u;
    v = (1-(j-1+xOffset*REFRACTION)/(WATERSIZE_X-3))*repeat_v;
    //shading = (float)fabs(xOffset)*SHADING;
    //glColor4f(0.69f,0.875f,0.9f, shading);
    glTexCoord2f(u, v);
    glVertex3f(i-WATERSIZE_Y/2.0f, j-WATERSIZE_X/2.0f, 0.0f);
}

/**
 * Calculates and draws values in the "water" array
 */
void display(void) {
    int i, j, tmp;
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(0, 0, spin_z-323);
    glRotatef(spin_x, 0, 1, 0);
    glRotatef(spin_y, 1, 0, 0);

	calcWater();

    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    /*glBegin(GL_POINTS);
    for(i = 1; i < WATERSIZE_Y; ++i) {
        for(j = 1; j < WATERSIZE_X; ++j) {
			glVertex3f(i-WATERSIZE_Y/2, j-WATERSIZE_X/2, water[buff_1][i][j]);
		}
	}
    glEnd();*/

    //with height map
    /*glBegin(GL_TRIANGLES);
    for(i = 1; i < WATERSIZE_Y-2; ++i){
        for(j = 1; j < WATERSIZE_X-2; ++j){
            glVertex3f(i-WATERSIZE_Y/2, j-WATERSIZE_X/2, water[buff_1][i][j]);
            glVertex3f(i-WATERSIZE_Y/2, (j+1)-WATERSIZE_X/2, water[buff_1][i][j+1]);
            glVertex3f((i+1)-WATERSIZE_Y/2, j-WATERSIZE_X/2, water[buff_1][i+1][j]);
            glVertex3f(i-WATERSIZE_Y/2, (j+1)-WATERSIZE_X/2, water[buff_1][i][j+1]);
            glVertex3f((i+1)-WATERSIZE_Y/2, (j+1)-WATERSIZE_X/2, water[buff_1][i+1][j+1]);
            glVertex3f((i+1)-WATERSIZE_Y/2, j-WATERSIZE_X/2, water[buff_1][i+1][j]);
        }
    }
    glEnd();*/

    glBegin(GL_TRIANGLES);
    for(i = 1; i < WATERSIZE_Y-2; ++i){
	    for(j = 1; j < WATERSIZE_X-2; ++j){
	        drawVert(i, j);
            drawVert(i+1, j);
            drawVert(i, j+1);
	        drawVert(i, j+1);
	        drawVert(i+1, j);
            drawVert(i+1, j+1);
        }
	}
	glEnd();

    //swap buffer
	tmp = buff_1;
    buff_1 = buff_2;
    buff_2 = tmp;

    glPopMatrix();
    glutSwapBuffers();
    glutPostRedisplay();
    frame++;
}

/**
 * Mouse inputs
 * @param button pressed mouse button
 * @param state pressed/unpressed mouse button
 * @param x x-coord of window
 * @param y y-coord of window
 */
void mouse(int button, int state, int x, int y) {
    int water_x, water_y;
    float tmpx, tmpy;
    currWidth = glutGet(GLUT_WINDOW_WIDTH);
    currHeight = glutGet(GLUT_WINDOW_HEIGHT);
    switch(button) {
        case 0:
            old_x = x - spin_x;
            old_y = y - spin_y;
            if(state == GLUT_DOWN){
                tmpx = x/currHeight * (WATERSIZE_X);
                tmpy = y/currWidth * (WATERSIZE_Y);
                water_x = (WATERSIZE_X) - (int)tmpy;
                water_y = (int)tmpx;
                //printf("%d : %d\n", water_x, water_y);
                if ((water_x > 0 && water_x < WATERSIZE_X-1) && (water_y > 0 && water_y < WATERSIZE_Y-1))
                    ripple(water_x, water_y, RIPPLE);
            }
            break;
        case 2:
            //old_y = y - spin_z;
            //move_z = (move_z ? 0 : 1);
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

/**
 * Mouse movement
 * @param x
 * @param y
 */
void motion(int x, int y) {
    int water_x, water_y;
    float tmpx, tmpy;
    if(!move_z) {
        spin_x = x - old_x;
        spin_y = y - old_y;
        tmpx = x/currHeight * WATERSIZE_X;
        tmpy = y/currWidth * WATERSIZE_Y;
        water_x = WATERSIZE_X - (int)tmpy;
        water_y = (int)tmpx;
        //printf("%d : %d\n", water_x, water_y);
        if ((water_x > 0 && water_x < WATERSIZE_X-1) && (water_y > 0 && water_y < WATERSIZE_Y-1))
            ripple(water_x, water_y, RIPPLE);
    } else {
        //spin_z = y - old_y;
    }
    glutPostRedisplay();
}

/**
 * Processing keyboard input
 * @param key pressed key
 * @param x
 * @param y
 */
void keyboard(unsigned char key, int x, int y){
    switch (key) {
        case 27:
            exit (0);
        case 'f':
            if (glutGet(GLUT_WINDOW_WIDTH) < glutGet(GLUT_SCREEN_WIDTH)) {
                glutFullScreen();
            }
            else{
                glutReshapeWindow(WIN_WIDTH, WIN_HEIGHT);
            }
            //currWidth = glutGet(GLUT_WINDOW_WIDTH);
            //currHeight = glutGet(GLUT_WINDOW_HEIGHT);
            break;
        case 'r':
            rain = (rain == 1) ? 0: 1;
            break;
        case 's':
            resetWaterGrid();
            break;
		default:
			break;
    }
}

/**
 * FUnction for glutIdleFunc(), creates "rain" and calculates FPS
 */
void idle(void) {
    int x, y, val, time;
    double fps;
    if(rain){
        if(!(++num % delay)) {
            x = (rand()%(WATERSIZE_X-3))+2;
            y = (rand()%(WATERSIZE_Y-3))+2;
            val = rand()%RIPPLE+50;
            delay = rand()%MAX_DELAY+10;
            ripple(x, y, val);
        }
    }

    glutPostRedisplay();

    time=glutGet(GLUT_ELAPSED_TIME);
    if (time - timebase > 1000) {
        fps = frame*1000.0/(time-timebase);
        timebase = time;
        frame = 0;
        //printf("%f\n", fps);
    }
}

/**
 * Init: window settings, lights, materials and textures
 */
void init(void) {
	int i, j;
	currHeight = WIN_HEIGHT;
	currWidth = WIN_WIDTH;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    //glShadeModel(GL_SMOOTH);
    //glShadeModel(GL_FLAT);

    textureID = loadTexture();
    resetWaterGrid();

    GLfloat ambientColor[] = {0.6f, 0.6f, 0.6f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

    //Add positioned light
    GLfloat lightColor0[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightPos0[] = {25.0f, 80.0f, 15.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

    //Add directed light
    GLfloat lightColor1[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightPos1[] = {-10.0f, 0.5f, -10.0f, 0.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);

    float MatAmbient[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    float MatDiffuse[] = { 0.0f, 0.6f, 0.7f, 1.0f };
    float MatSpecular[] = { 1.0f, 1.0f, 1.0f, 0.1f };
    float MatShininess = 1.5f;
    //float black[] = {0.0f,0.0f,0.0f,1.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, MatAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MatDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, MatSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, MatShininess);
    //glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    //glColor4f(0.7f,0.87f,0.9f, BLEND_FACTOR);
    //glColor4f(0.0f,0.1f,0.9f, BLEND_FACTOR);
    glColor4f(0.3f,0.45f,0.85f, BLEND_FACTOR);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
}

int main(int argc, char** argv) {
    srand(time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Water ripples");

    glutDisplayFunc(display);

    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    init();
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
