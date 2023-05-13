/******************************************************************************
 *
 * Computer Graphics Programming 2020 Project Template v1.0 (11/04/2021)
 *
 * Based on: Animation Controller v1.0 (11/04/2021)
 *
 * This template provides a basic FPS-limited render loop for an animated scene,
 * plus keyboard handling for smooth game-like control of an object such as a
 * character or vehicle.
 *
 * A simple static lighting setup is provided via initLights(), which is not
 * included in the animationalcontrol.c template. There are no other changes.
 *
 ******************************************************************************/

#include <Windows.h>
#include <freeglut.h>
#include <math.h>
#include <stdio.h>


 /******************************************************************************
  * Animation & Timing Setup
  ******************************************************************************/

  // Target frame rate (number of Frames Per Second).
#define TARGET_FPS 60				

// Ideal time each frame should be displayed for (in milliseconds).
const unsigned int FRAME_TIME = 1000 / TARGET_FPS;

// Frame time in fractional seconds.
// Note: This is calculated to accurately reflect the truncated integer value of
// FRAME_TIME, which is used for timing, rather than the more accurate fractional
// value we'd get if we simply calculated "FRAME_TIME_SEC = 1.0f / TARGET_FPS".
const float FRAME_TIME_SEC = (1000 / TARGET_FPS) / 1000.0f;

// Time we started preparing the current frame (in milliseconds since GLUT was initialized).
unsigned int frameStartTime = 0;

/******************************************************************************
 * Some Simple Definitions of Motion
 ******************************************************************************/

#define MOTION_NONE 0				// No motion.
#define MOTION_CLOCKWISE -1			// Clockwise rotation.
#define MOTION_ANTICLOCKWISE 1		// Anticlockwise rotation.
#define MOTION_BACKWARD -1			// Backward motion.
#define MOTION_FORWARD 1			// Forward motion.
#define MOTION_LEFT -1				// Leftward motion.
#define MOTION_RIGHT 1				// Rightward motion.
#define MOTION_DOWN -1				// Downward motion.
#define MOTION_UP 1					// Upward motion.

 // Represents the motion of an object on four axes (Yaw, Surge, Sway, and Heave).
 // 
 // You can use any numeric values, as specified in the comments for each axis. However,
 // the MOTION_ definitions offer an easy way to define a "unit" movement without using
 // magic numbers (e.g. instead of setting Surge = 1, you can set Surge = MOTION_FORWARD).
 //
typedef struct {
	int Yaw;		// Turn about the Z axis	[<0 = Clockwise, 0 = Stop, >0 = Anticlockwise]
	int Surge;		// Move forward or back		[<0 = Backward,	0 = Stop, >0 = Forward]
	int Sway;		// Move sideways (strafe)	[<0 = Left, 0 = Stop, >0 = Right]
	int Heave;		// Move vertically			[<0 = Down, 0 = Stop, >0 = Up]
} motionstate4_t;


// MY FUNCTIONS
void drawRectangle3D(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2);
void drawSea();
void drawScene();
void playerControls();
void drawCylinder();
void drawHelicopter();
// returns pointer to image data
GLubyte* loadImage(float* imageWidth, float* imageHeight, char* fileName);


//cube vertices
GLfloat vertices[][3] = { {-1.0,-1.0,-1.0}, {1.0,-1.0,-1.0}, {1.0,1.0,-1.0}, {-
1.0,1.0,-1.0},
{-1.0,-1.0, 1.0}, {1.0,-1.0, 1.0},
{1.0,1.0, 1.0}, {-1.0,1.0, 1.0} };


// window dimensions
GLint windowWidth = 500;
GLint windowHeight = 400;

typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
} pos;

// sea mesh size
const int seaSizeX = 100;
const int seaSizeZ = 100;
const float seaMaxY = 0.1f;

GLubyte* skyTexture;

// scene is scaled to 0.1 (10x10) 
// change to 1 when ready (100x100)
const float sceneMultiplier = 0.1f;
float seaRadius;
GLfloat cylinderHeight = 30;

// helicopter engine is initially set to off
int heliStart = 0;
// toggle side view
int sideView = 0;

pos heliPos = {0, 0.5f, 0};
pos prevHeliPos = { 0, 0, 0 };

GLfloat heliRadius = 0.1;
GLfloat heliTailLength = 0.15;
GLfloat heliLegLength = 0.05;
GLfloat heliAngle = 0;

// angle 1 for back propellor
GLfloat propellorAngle = 0;
// angle 2 for top propellor
GLfloat propellorAngle2 = 0;

// degrees per frame for back propellor
GLfloat propellorSpeed = -720;

double surgeDirection = 0;
double swayDirection = 0;

pos camPos = { 0, 1.0f, 2 }; // away from the viewer with 2 on the z acis {0, 1.0f, 2}
GLfloat camAngle;
GLfloat camDistance = 10.0f;

const float moveSpeed = 2.0f; // metres per sec
GLfloat deltaTime;
GLfloat oldDeltaTime;


/******************************************************************************
 * Keyboard Input Handling Setup
 ******************************************************************************/

 // Represents the state of a single keyboard key.Represents the state of a single keyboard key.
typedef enum {
	KEYSTATE_UP = 0,	// Key is not pressed.
	KEYSTATE_DOWN		// Key is pressed down.
} keystate_t;

// Represents the states of a set of keys used to control an object's motion.
typedef struct {
	keystate_t MoveForward;
	keystate_t MoveBackward;
	keystate_t MoveLeft;
	keystate_t MoveRight;
	keystate_t MoveUp;
	keystate_t MoveDown;
	keystate_t TurnLeft;
	keystate_t TurnRight;
} motionkeys_t;

// Current state of all keys used to control our "player-controlled" object's motion.
motionkeys_t motionKeyStates = {
	KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP,
	KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP };

// How our "player-controlled" object should currently be moving, solely based on keyboard input.
//
// Note: this may not represent the actual motion of our object, which could be subject to
// other controls (e.g. mouse input) or other simulated forces (e.g. gravity).
motionstate4_t keyboardMotion = { MOTION_NONE, MOTION_NONE, MOTION_NONE, MOTION_NONE };

// Define all character keys used for input (add any new key definitions here).
// Note: USE ONLY LOWERCASE CHARACTERS HERE. The keyboard handler provided converts all
// characters typed by the user to lowercase, so the SHIFT key is ignored.

#define KEY_MOVE_FORWARD	'w'
#define KEY_MOVE_BACKWARD	's'
#define KEY_MOVE_LEFT		'a'
#define KEY_MOVE_RIGHT		'd'
#define KEY_RENDER_FILL		'l'
#define KEY_SIDE_VIEW		'x'
#define KEY_EXIT			27 // Escape key.

// Define all GLUT special keys used for input (add any new key definitions here).

#define SP_KEY_MOVE_UP		GLUT_KEY_UP
#define SP_KEY_MOVE_DOWN	GLUT_KEY_DOWN
#define SP_KEY_TURN_LEFT	GLUT_KEY_LEFT
#define SP_KEY_TURN_RIGHT	GLUT_KEY_RIGHT

/******************************************************************************
 * GLUT Callback Prototypes
 ******************************************************************************/

void display(void);
void reshape(int width, int h);
void keyPressed(unsigned char key, int x, int y);
void specialKeyPressed(int key, int x, int y);
void keyReleased(unsigned char key, int x, int y);
void specialKeyReleased(int key, int x, int y);
void idle(void);

/******************************************************************************
 * Animation-Specific Function Prototypes (add your own here)
 ******************************************************************************/

void main(int argc, char** argv);
void init(void);
void think(void);
void initLights(void);


/******************************************************************************
 * Animation-Specific Setup (Add your own definitions, constants, and globals here)
 ******************************************************************************/

 // Render objects as filled polygons (1) or wireframes (0). Default filled.
int renderFillEnabled = 1;

/******************************************************************************
 * Entry Point (don't put anything except the main function here)
 ******************************************************************************/

void main(int argc, char** argv)
{
	// Initialize the OpenGL window.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1000, 800);
	glutCreateWindow("Animation");

	// Set up the scene.
	init();

	// Disable key repeat (keyPressed or specialKeyPressed will only be called once when a key is first pressed).
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	// Register GLUT callbacks.
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyPressed);
	glutSpecialFunc(specialKeyPressed);
	glutKeyboardUpFunc(keyReleased);
	glutSpecialUpFunc(specialKeyReleased);
	glutIdleFunc(idle);

	// Record when we started rendering the very first frame (which should happen after we call glutMainLoop).
	frameStartTime = (unsigned int)glutGet(GLUT_ELAPSED_TIME);

	// Enter the main drawing loop (this will never return).
	glutMainLoop();
}

/******************************************************************************
 * GLUT Callbacks (don't add any other functions here)
 ******************************************************************************/

 /*
	 Called when GLUT wants us to (re)draw the current animation frame.

	 Note: This function must not do anything to update the state of our simulated
	 world. Animation (moving or rotating things, responding to keyboard input,
	 etc.) should only be performed within the think() function provided below.
 */
void display(void)
{
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// load the identity matrix into the model view matrix
	glLoadIdentity();

	if (renderFillEnabled) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	// camera setup
	gluLookAt(camPos.x, camPos.y, camPos.z,
		heliPos.x, heliPos.y, heliPos.z,
		0, 1, 0);

		// Draw the positive side of the lines x,y,z
		// lines coming out of origin
		glBegin(GL_LINES);
		glColor3f(0.0f, 1.0f, 0.0f);                // Green for x axis
		glVertex3f(0, 0, 0);
		glVertex3f(10, 0, 0);
		glColor3f(1.0f, 0.0f, 0.0f);                // Red for y axis
		glVertex3f(0, 0, 0);
		glVertex3f(0, 10, 0);
		glColor3f(0.0f, 0.0f, 1.0f);                // Blue for z axis
		glVertex3f(0, 0, 0);
		glVertex3f(0, 0, 10);
		glEnd();

		// DRAW SEA / SKY / ROCKS
		drawScene();
		
		drawHelicopter();

	glutSwapBuffers();

}



/*
	Called when the OpenGL window has been resized.
*/
void reshape(int width, int h)
{
	// update the new width
	windowWidth = width;
	// update the new height
	windowHeight = h;
	// update the viewport to still be all of the window
	glViewport(0, 0, windowWidth, windowHeight);
	// change into projection mode so that we can change the camera properties
	glMatrixMode(GL_PROJECTION);
	// load the identity matrix into the projection matrix
	glLoadIdentity();
	// gluPerspective(fovy, aspect, near, far)
	gluPerspective(45, (float)windowWidth / (float)windowHeight, 1, 20);
	// change into model-view mode so that we can change the object positions
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

}

/*
	Called each time a character key (e.g. a letter, number, or symbol) is pressed.
*/
void keyPressed(unsigned char key, int x, int y)
{
	switch (tolower(key)) {

		/*
			Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION

			Whenever one of our movement keys is pressed, we do two things:
			(1) Update motionKeyStates to record that the key is held down. We use
				this later in the keyReleased callback.
			(2) Update the relevant axis in keyboardMotion to set the new direction
				we should be moving in. The most recent key always "wins" (e.g. if
				you're holding down KEY_MOVE_LEFT then also pressed KEY_MOVE_RIGHT,
				our object will immediately start moving right).
		*/
	case KEY_MOVE_FORWARD:
		motionKeyStates.MoveForward = KEYSTATE_DOWN;
		keyboardMotion.Surge = MOTION_FORWARD;
		break;
	case KEY_MOVE_BACKWARD:
		motionKeyStates.MoveBackward = KEYSTATE_DOWN;
		keyboardMotion.Surge = MOTION_BACKWARD;
		break;
	case KEY_MOVE_LEFT:
		motionKeyStates.MoveLeft = KEYSTATE_DOWN;
		keyboardMotion.Sway = MOTION_LEFT;
		break;
	case KEY_MOVE_RIGHT:
		motionKeyStates.MoveRight = KEYSTATE_DOWN;
		keyboardMotion.Sway = MOTION_RIGHT;
		break;

		/*
			Other Keyboard Functions (add any new character key controls here)

			Rather than using literals (e.g. "t" for spotlight), create a new KEY_
			definition in the "Keyboard Input Handling Setup" section of this file.
			For example, refer to the existing keys used here (KEY_MOVE_FORWARD,
			KEY_MOVE_LEFT, KEY_EXIT, etc).
		*/
	case KEY_RENDER_FILL:
		renderFillEnabled = !renderFillEnabled;
		break;
	case KEY_EXIT:
		exit(0);
		break;

	case KEY_SIDE_VIEW:
		if (sideView) {
			sideView = !sideView;
		}
		else {
			sideView = 1;
		}
	}
}

/*
	Called each time a "special" key (e.g. an arrow key) is pressed.
*/
void specialKeyPressed(int key, int x, int y)
{
	switch (key) {

		/*
			Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION

			This works as per the motion keys in keyPressed.
		*/
	case SP_KEY_MOVE_UP:
		motionKeyStates.MoveUp = KEYSTATE_DOWN;
		keyboardMotion.Heave = MOTION_UP;
		break;
	case SP_KEY_MOVE_DOWN:
		motionKeyStates.MoveDown = KEYSTATE_DOWN;
		keyboardMotion.Heave = MOTION_DOWN;
		break;
	case SP_KEY_TURN_LEFT:
		motionKeyStates.TurnLeft = KEYSTATE_DOWN;
		keyboardMotion.Yaw = MOTION_ANTICLOCKWISE;
		break;
	case SP_KEY_TURN_RIGHT:
		motionKeyStates.TurnRight = KEYSTATE_DOWN;
		keyboardMotion.Yaw = MOTION_CLOCKWISE;
		break;

		/*
			Other Keyboard Functions (add any new special key controls here)

			Rather than directly using the GLUT constants (e.g. GLUT_KEY_F1), create
			a new SP_KEY_ definition in the "Keyboard Input Handling Setup" section of
			this file. For example, refer to the existing keys used here (SP_KEY_MOVE_UP,
			SP_KEY_TURN_LEFT, etc).
		*/
	}
}

/*
	Called each time a character key (e.g. a letter, number, or symbol) is released.
*/
void keyReleased(unsigned char key, int x, int y)
{
	switch (tolower(key)) {

		/*
			Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION

			Whenever one of our movement keys is released, we do two things:
			(1) Update motionKeyStates to record that the key is no longer held down;
				we need to know when we get to step (2) below.
			(2) Update the relevant axis in keyboardMotion to set the new direction
				we should be moving in. This gets a little complicated to ensure
				the controls work smoothly. When the user releases a key that moves
				in one direction (e.g. KEY_MOVE_RIGHT), we check if its "opposite"
				key (e.g. KEY_MOVE_LEFT) is pressed down. If it is, we begin moving
				in that direction instead. Otherwise, we just stop moving.
		*/
	case KEY_MOVE_FORWARD:
		motionKeyStates.MoveForward = KEYSTATE_UP;
		keyboardMotion.Surge = (motionKeyStates.MoveBackward == KEYSTATE_DOWN) ? MOTION_BACKWARD : MOTION_NONE;
		break;
	case KEY_MOVE_BACKWARD:
		motionKeyStates.MoveBackward = KEYSTATE_UP;
		keyboardMotion.Surge = (motionKeyStates.MoveForward == KEYSTATE_DOWN) ? MOTION_FORWARD : MOTION_NONE;
		break;
	case KEY_MOVE_LEFT:
		motionKeyStates.MoveLeft = KEYSTATE_UP;
		keyboardMotion.Sway = (motionKeyStates.MoveRight == KEYSTATE_DOWN) ? MOTION_RIGHT : MOTION_NONE;
		break;
	case KEY_MOVE_RIGHT:
		motionKeyStates.MoveRight = KEYSTATE_UP;
		keyboardMotion.Sway = (motionKeyStates.MoveLeft == KEYSTATE_DOWN) ? MOTION_LEFT : MOTION_NONE;
		break;

		/*
			Other Keyboard Functions (add any new character key controls here)

			Note: If you only care when your key is first pressed down, you don't have to
			add anything here. You only need to put something in keyReleased if you care
			what happens when the user lets go, like we do with our movement keys above.
			For example: if you wanted a spotlight to come on while you held down "t", you
			would need to set a flag to turn the spotlight on in keyPressed, and update the
			flag to turn it off in keyReleased.
		*/
	}
}

/*
	Called each time a "special" key (e.g. an arrow key) is released.
*/
void specialKeyReleased(int key, int x, int y)
{
	switch (key) {
		/*
			Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION

			This works as per the motion keys in keyReleased.
		*/
	case SP_KEY_MOVE_UP:
		motionKeyStates.MoveUp = KEYSTATE_UP;
		keyboardMotion.Heave = (motionKeyStates.MoveDown == KEYSTATE_DOWN) ? MOTION_DOWN : MOTION_NONE;
		break;
	case SP_KEY_MOVE_DOWN:
		motionKeyStates.MoveDown = KEYSTATE_UP;
		keyboardMotion.Heave = (motionKeyStates.MoveUp == KEYSTATE_DOWN) ? MOTION_UP : MOTION_NONE;
		break;
	case SP_KEY_TURN_LEFT:
		motionKeyStates.TurnLeft = KEYSTATE_UP;
		keyboardMotion.Yaw = (motionKeyStates.TurnRight == KEYSTATE_DOWN) ? MOTION_CLOCKWISE : MOTION_NONE;
		break;
	case SP_KEY_TURN_RIGHT:
		motionKeyStates.TurnRight = KEYSTATE_UP;
		keyboardMotion.Yaw = (motionKeyStates.TurnLeft == KEYSTATE_DOWN) ? MOTION_ANTICLOCKWISE : MOTION_NONE;
		break;

		/*
			Other Keyboard Functions (add any new special key controls here)

			As per keyReleased, you only need to handle the key here if you want something
			to happen when the user lets go. If you just want something to happen when the
			key is first pressed, add you code to specialKeyPressed instead.
		*/
	}
}

/*
	Called by GLUT when it's not rendering a frame.

	Note: We use this to handle animation and timing. You shouldn't need to modify
	this callback at all. Instead, place your animation logic (e.g. moving or rotating
	things) within the think() method provided with this template.
*/
void idle(void)
{
	// Wait until it's time to render the next frame.

	unsigned int frameTimeElapsed = (unsigned int)glutGet(GLUT_ELAPSED_TIME) - frameStartTime;
	if (frameTimeElapsed < FRAME_TIME)
	{
		// This frame took less time to render than the ideal FRAME_TIME: we'll suspend this thread for the remaining time,
		// so we're not taking up the CPU until we need to render another frame.
		unsigned int timeLeft = FRAME_TIME - frameTimeElapsed;
		Sleep(timeLeft);
	}

	// Begin processing the next frame.

	frameStartTime = glutGet(GLUT_ELAPSED_TIME); // Record when we started work on the new frame.

	think(); // Update our simulated world before the next call to display().

	glutPostRedisplay(); // Tell OpenGL there's a new frame ready to be drawn.
}

/******************************************************************************
 * Animation-Specific Functions (Add your own functions at the end of this section)
 ******************************************************************************/

 /*
	 Initialise OpenGL and set up our scene before we begin the render loop.
 */
void init(void)
{
	float skyWidth = 7583;
	float skyHeight = 1960;
	seaRadius = (seaSizeX * sceneMultiplier)/2;
	skyTexture = loadImage(&skyWidth, &skyHeight, "/assets/sky.ppm");
	glTexImage2D(GL_TEXTURE_2D, 0, 3, skyWidth, skyHeight, 0,
		GL_RGB, GL_UNSIGNED_BYTE, skyTexture);


	initLights();

	// enable depth testing
	glEnable(GL_DEPTH_TEST);
	// set background color to be black
	glClearColor(0, 0, 0, 1.0);
	glColor3f(1.0, 1.0, 1.0);
	// load the identity matrix into the projection matrix
	glMatrixMode(GL_PROJECTION);

	glShadeModel(GL_FLAT);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	// set window mode to 3D perspective
	// gluPerspective(fovy, aspect, near, far);
	gluPerspective(60, (double)windowWidth / (double)windowHeight, 1, 200);

	// change into model-view mode so that we can change the object positions
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/*
 * Loads a PPM image and returns a pointer to the image data
 */
GLubyte* loadImage(float* imageWidth, float* imageHeight, char* fileName)
{
	// the ID of the image file
	FILE* fileID;

	// maxValue
	int  maxValue;

	// total number of pixels in the image
	int  totalPixels;

	// temporary character
	char tempChar;

	// counter variable for the current pixel in the image
	int i;

	// array for reading in header information
	char headerLine[100];

	// if the original values are larger than 255
	float RGBScaling;

	// temporary variables for reading in the red, green and blue data of each pixel
	int red, green, blue;

	// open the image file for reading - note this is hardcoded would be better to provide a parameter which
	//is the file name. There are 3 PPM files you can try out mount03, sky08 and sea02.
	fileID = fopen(fileName, "r");

	// read in the first header line
	//    - "%[^\n]"  matches a string of all characters not equal to the new line character ('\n')
	//    - so we are just reading everything up to the first line break
	fscanf(fileID, "%[^\n] ", headerLine);

	// make sure that the image begins with 'P3', which signifies a PPM file
	if ((headerLine[0] != 'P') || (headerLine[1] != '3'))
	{
		printf("This is not a PPM file!\n");
		exit(0);
	}

	// we have a PPM file
	printf("This is a PPM file\n");

	// read in the first character of the next line
	fscanf(fileID, "%c", &tempChar);

	// while we still have comment lines (which begin with #)
	while (tempChar == '#')
	{
		// read in the comment
		fscanf(fileID, "%[^\n] ", headerLine);

		// print the comment
		printf("%s\n", headerLine);

		// read in the first character of the next line
		fscanf(fileID, "%c", &tempChar);
	}

	// the last one was not a comment character '#', so we need to put it back into the file stream (undo)
	ungetc(tempChar, fileID);

	// read in the image hieght, width and the maximum value
	fscanf(fileID, "%f %f %d", imageWidth, imageHeight, &maxValue);

	// print out the information about the image file
	printf("%d rows  %d columns  max value= %d\n", (int)*imageHeight, (int)*imageWidth, maxValue);

	// compute the total number of pixels in the image
	totalPixels = (int)(*imageWidth) * (int)(*imageHeight);

	// allocate enough memory for the image  (3*) because of the RGB data
	GLubyte* imageData = malloc(3 * sizeof(GLuint) * totalPixels);

	// determine the scaling for RGB values
	RGBScaling = 255.0 / maxValue;
	// if the maxValue is 255 then we do not need to scale the 
	//    image data values to be in the range or 0 to 255
	if (maxValue == 255)
	{
		for (i = 0; i < totalPixels; i++)
		{
			// read in the current pixel from the file
			fscanf(fileID, "%d %d %d", &red, &green, &blue);

			// store the red, green and blue data of the current pixel in the data array
			imageData[3 * totalPixels - 3 * i - 3] = red;
			imageData[3 * totalPixels - 3 * i - 2] = green;
			imageData[3 * totalPixels - 3 * i - 1] = blue;
		}
	}
	else  // need to scale up the data values
	{
		for (i = 0; i < totalPixels; i++)
		{
			// read in the current pixel from the file
			fscanf(fileID, "%d %d %d", &red, &green, &blue);

			// store the red, green and blue data of the current pixel in the data array
			imageData[3 * totalPixels - 3 * i - 3] = red * RGBScaling;
			imageData[3 * totalPixels - 3 * i - 2] = green * RGBScaling;
			imageData[3 * totalPixels - 3 * i - 1] = blue * RGBScaling;
		}
	}

	return imageData;
}

void drawRectangle3D(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2) {
	// draw rectangle
	glBegin(GL_QUADS);

	// front face
	glVertex3f(x1, y1, z1);
	glVertex3f(x2, y1, z1);
	glVertex3f(x2, y2, z1);
	glVertex3f(x1, y2, z1);

	// back face
	glVertex3f(x1, y1, z2);
	glVertex3f(x2, y1, z2);
	glVertex3f(x2, y2, z2);
	glVertex3f(x1, y2, z2);

	// top face
	glVertex3f(x1, y2, z1);
	glVertex3f(x2, y2, z1);
	glVertex3f(x2, y2, z2);
	glVertex3f(x1, y2, z2);

	// bottom face
	glVertex3f(x1, y1, z1);
	glVertex3f(x2, y1, z1);
	glVertex3f(x2, y1, z2);
	glVertex3f(x1, y1, z2);

	// left face
	glVertex3f(x1, y1, z1);
	glVertex3f(x1, y2, z1);
	glVertex3f(x1, y2, z2);
	glVertex3f(x1, y1, z2);

	// right face
	glVertex3f(x2, y1, z1);
	glVertex3f(x2, y2, z1);
	glVertex3f(x2, y2, z2);
	glVertex3f(x2, y1, z2);

	glEnd();
}

void drawHelicopter() {
	GLUquadricObj* quadricPtr;

	// helicopter base
	glPushMatrix();

	glTranslatef(heliPos.x, heliPos.y, heliPos.z);
	glRotatef(heliAngle, 0.0f, 1.0f, 0.0f);
	quadricPtr = gluNewQuadric();

	// set to greyish blue
	glColor3f(126.0f / 255.0f, 148.0f / 255.0f, 142.0f / 255.0f);
	gluQuadricDrawStyle(quadricPtr, GLU_FILL);
	gluSphere(quadricPtr, heliRadius, 10, 5);

	// helicopter legs
	glPushMatrix();
	glTranslatef(0, -heliRadius + 0.01, 0);
	// right leg
	drawRectangle3D(0.025, 0.01, 0.0, 0.05, -0.02, 0.01);
	// left leg
	drawRectangle3D(-0.025, 0.01, 0.0, -0.05, -0.02, 0.01);


	glPopMatrix();

	// helicopter tail
	glPushMatrix();

	glTranslatef(0, 0, heliRadius);

	gluCylinder(quadricPtr, 0.02, 0.01, heliTailLength, 5, 5);

	// helicopter back propellor
	glPushMatrix();
	// set to blue 
	glColor3f(75.0f / 255.0f, 222.0f / 255.0f, 222.0f / 255.0f);
	glTranslatef(0.015, 0, heliTailLength);
	glRotatef(propellorAngle, 1, 0, 0);

	// horizontal propellor
	drawRectangle3D(0.0, 0.0, -0.055, 0.01, 0.01, 0.055);
	// vertical propellor
	drawRectangle3D(0.0, -0.055, 0.0, 0.01, 0.055, 0.01);

	glPopMatrix();

	glPopMatrix();

	// helicopter top propellor
	glPushMatrix();
	glTranslatef(0, heliRadius, 0);
	glRotatef(propellorAngle2, 0, 1, 0);

	// horizontal propellor
	drawRectangle3D(0.0, 0.0, -.15, 0.01, 0.015, 0.15);
	// horizontal propellor
	drawRectangle3D(-.15, 0, 0.0, .15, 0.015, 0.01);

	glPopMatrix();


	gluDeleteQuadric(quadricPtr);

	glPopMatrix();
}

void drawSea(int sizeX, int sizeZ, float time, float maxY) {
	float centerX = (float)sizeX / 2.0f;
	float centerZ = (float)sizeZ / 2.0f;
	float amplitude = maxY;
	float frequency = 2.0f;
	int numVertices = sizeX * sizeZ;
	int numIndices = (sizeX - 1) * (sizeZ - 1) * 4;
	float* vertices = (float*)malloc(numVertices * 3 * sizeof(float));
	int* indices = (int*)malloc(numIndices * sizeof(int));
	int index = 0;
	for (int z = 0; z < sizeZ; z++) {
		for (int x = 0; x < sizeX; x++) {
			float vertexX = ((float)x - centerX) * sceneMultiplier;
			float vertexZ = ((float)z - centerZ) * sceneMultiplier;
			float vertexY = amplitude * sinf(frequency * vertexX + time) * sinf(frequency * vertexZ + time);
			vertices[index++] = vertexX;
			vertices[index++] = vertexY;
			vertices[index++] = vertexZ;
		}
	}
	index = 0;
	for (int z = 0; z < sizeZ - 1; z++) {
		for (int x = 0; x < sizeX - 1; x++) {
			int vertexIndex = z * sizeX + x;
			indices[index++] = vertexIndex;
			indices[index++] = vertexIndex + sizeX;
			indices[index++] = vertexIndex + sizeX + 1;
			indices[index++] = vertexIndex + 1;
		}
	}
	glBegin(GL_QUADS);
	for (int i = 0; i < numIndices; i++) {
		int vertexIndex = indices[i] * 3;
		glVertex3f(vertices[vertexIndex], vertices[vertexIndex + 1], vertices[vertexIndex + 2]);
	}

	glEnd();
	free(vertices);
	free(indices);
}

void drawCylinder() {
	const float pi = 3.14159265358979323846f;
	const int sides = 12;
	unsigned int numberOfSegments = 64;
	float angleIncrement = (2.0f * pi) / (numberOfSegments);
	float textureCoordinateIncrement = 1.0f / (numberOfSegments);

	glBindTexture(GL_TEXTURE_2D, skyTexture); // bind the sky texture

	glBegin(GL_QUAD_STRIP);
	for (unsigned int i = 0; i <= numberOfSegments; ++i)
	{
		float c = seaRadius * cos(angleIncrement * i);
		float s = seaRadius * sin(angleIncrement	* i);

		glTexCoord2f(textureCoordinateIncrement * i, 0); glVertex3f(c, 0.0f, s);
		glTexCoord2f(textureCoordinateIncrement * i, 2.0f); glVertex3f(c, 2.0f, s);
	}
	glEnd();
}

// draw sea and mountain range and sky and all bg related objects
void drawScene() {
	static float lastTime = 0.0f;
	float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	float deltaTime = currentTime - lastTime;
	lastTime = currentTime;
	drawSea(seaSizeX, seaSizeZ, currentTime, seaMaxY); // pass in the current time value
	drawCylinder();
}

void playerControls() {
	if (keyboardMotion.Yaw || keyboardMotion.Heave || keyboardMotion.Surge || keyboardMotion.Sway) {
		heliStart = 1;

	}

	if (keyboardMotion.Yaw != MOTION_NONE) {
		// Update helicopter angle
		heliAngle += keyboardMotion.Yaw * (moveSpeed + 50) * FRAME_TIME_SEC;
		heliAngle = fmod(heliAngle, 360.0f);

		// Calculate yaw angle in radians
		double yawAngle = heliAngle * (3.14 / 180.0f);

		// Calculate new camera position
		double distanceXZ = sqrt(pow(heliPos.z - camPos.z, 2) + pow(heliPos.x - camPos.x, 2));
		camPos.x = heliPos.x + sin(yawAngle) * distanceXZ;
		camPos.z = heliPos.z + cos(yawAngle) * distanceXZ;

		// Adjust surge and sway according to yaw angle
		surgeDirection = sin(yawAngle);
		swayDirection = cos(yawAngle);
	}

	if (keyboardMotion.Surge != MOTION_NONE) {
		// Move helicopter and camera
		double moveDistance = keyboardMotion.Surge * moveSpeed * FRAME_TIME_SEC;
		heliPos.x -= moveDistance * sin(heliAngle * (3.14 / 180.0f));
		heliPos.z -= moveDistance * cos(heliAngle * (3.14 / 180.0f));
		camPos.x -= moveDistance * sin(heliAngle * (3.14 / 180.0f));
		camPos.z -= moveDistance * cos(heliAngle * (3.14 / 180.0f));
	}

	if (keyboardMotion.Sway != MOTION_NONE) {
		// Move helicopter and camera left/right
		double moveDistance = keyboardMotion.Sway * moveSpeed * FRAME_TIME_SEC;
		heliPos.x += moveDistance * cos(heliAngle * (3.14 / 180.0f));
		heliPos.z -= moveDistance * sin(heliAngle * (3.14 / 180.0f));
		camPos.x += moveDistance * cos(heliAngle * (3.14 / 180.0f));
		camPos.z -= moveDistance * sin(heliAngle * (3.14 / 180.0f));
	}

	if (keyboardMotion.Heave != MOTION_NONE) {
		// Move helicopter and camera
		double heave = keyboardMotion.Heave * moveSpeed * FRAME_TIME_SEC;
		heliPos.y += heave;
		camPos.y = heliPos.y + 0.5f;
	}
}

/*
	Advance our animation by FRAME_TIME milliseconds.

	Note: Our template's GLUT idle() callback calls this once before each new
	frame is drawn, EXCEPT the very first frame drawn after our application
	starts. Any setup required before the first frame is drawn should be placed
	in init().
*/
void think(void)
{

	//TODO helicopter start and stop animation 
	if (heliStart) {
		propellorAngle += propellorSpeed * FRAME_TIME_SEC;
		propellorAngle2 += propellorSpeed * 1.5 * FRAME_TIME_SEC;
	}
	else {
		// decrement propellor angle until 0
		if (propellorAngle > 0) {
			propellorAngle -= propellorSpeed * 0.5 * FRAME_TIME_SEC;
		}
		if (propellorAngle2 > 0) {
			propellorAngle2 -= propellorSpeed * 0.5 * FRAME_TIME_SEC;
		}

	}

	// FOR DEBUGGING
	if (sideView) {
		camPos.x = 2;
		camPos.z = 0;
	}

	/*
		Keyboard motion handler: complete this section to make your "player-controlled"
		object respond to keyboard input.
	*/
	playerControls();
	// collision detection for heli and sea (sea max y)
	if (heliPos.y - (heliRadius + heliLegLength) < seaMaxY) {
		heliPos.y = seaMaxY+heliRadius+heliLegLength;
	}

}


/*
	Initialise OpenGL lighting before we begin the render loop.

	Note (advanced): If you're using dynamic lighting (e.g. lights that move around, turn on or
	off, or change colour) you may want to replace this with a drawLights function that gets called
	at the beginning of display() instead of init().
*/
void initLights(void)
{
	// Simple lighting setup
	GLfloat globalAmbient[] = { 0.4f, 0.4f, 0.4f, 1 };
	GLfloat lightPosition[] = { 5.0f, 5.0f, 5.0f, 1.0f };
	GLfloat ambientLight[] = { 0, 0, 0, 1 };
	GLfloat diffuseLight[] = { 1, 1, 1, 1 };
	GLfloat specularLight[] = { 1, 1, 1, 1 };

	// Configure global ambient lighting.
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

	// Configure Light 0.
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

	// Enable lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// Make GL normalize the normal vectors we supply.
	glEnable(GL_NORMALIZE);

	// Enable use of simple GL colours as materials.
	glEnable(GL_COLOR_MATERIAL);
}

/******************************************************************************/