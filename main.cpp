#include <iostream>
#include <sstream>
#include <time.h>
#include <stdio.h>
#include <sys/sysinfo.h>

#ifndef JNI_COMPATIBLE
#	include <GL/glew.h>
#	include <GL/glut.h>
#else
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>

#	include <stdio.h>
#	include <stdlib.h>
#	include <math.h>
#endif

#include "camera.h"
#include "bspTree.h"
#include "bspNode.h"
#include "vectors.h"
#include "mesh.h"
#include "map.h"
#include "timer.h"
#include "actor.h"
#include "glsl/glsl.h"
#include "metaballs/cubeGrid.h"
#include "metaballs/metaball.h"

using namespace std;

int mapPolys;
static C_Map map;
C_Vertex lightPosition;
C_MeshGroup cube;

/// Global variables
ESMatrix globalViewMatrix, globalProjectionMatrix, globalMVPMatrix;

C_GLShaderManager *shaderManager = NULL;
C_TextureManager *textureManager = NULL;

char MAX_THREADS = 0;

C_GLShader *bspShader = NULL;
C_GLShader *basicShader = NULL;
C_GLShader *pointShader = NULL;
C_GLShader *wallShader = NULL;

/// Camera and frustum
C_Camera camera;
static C_Frustum frustum;
static C_Party party;

/// window stuff
static int winID;
static int windowWidth = 800;
static int windowHeight = 500;
static int windowPositionX = 200;
static int windowPositionY = 200;

/// movement vars
static float speed = 7.0f;

static bool frustumCulling = true;
static int bspRenderingType = 0;

static C_Vector3 center(0.0f , 0.0f , 0.0f);

/// Timer vars
C_Timer timer;
float start = timer.GetTime ();
static float timeElapsed = 0.0f;
static float fps;

/// Metaballs
static C_CubeGrid *grid;
static C_Metaball metaball[3];

static void CountFPS (void);

static void
Initializations(void)
{
	/// Find number of cores available
   MAX_THREADS = get_nprocs();
   printf("Detected %d cpu cores.\n", MAX_THREADS);

   printf("EPSILON: %g\n", EPSILON);
   printf("GREATEST_FLOAT: %g\n", GREATEST_FLOAT);
   printf("SMALLEST_FLOAT: %g\n", SMALLEST_FLOAT);
   printf("FLT_MAX: %g\n", FLT_MAX);
   printf("FLT_MIN: %g\n", FLT_MIN);

	/// Set clear color
   #ifdef DRAW_BSP_GEOMETRY
	glClearColor(0.3671875f , 0.15234375f , 0.8359375f , 1.0f);
	#else
//	glClearColor(0.0, 0.0, 0.0, 1.0f);
   #endif

	/// Backface culling
	#ifdef DRAW_BSP_GEOMETRY
	glDisable(GL_CULL_FACE);
	#else
	glEnable(GL_CULL_FACE);
	#endif
//	glFrontFace(GL_CW);
//	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glPointSize(10.0f);
//	glShadeModel(GL_SMOOTH);

	/// XXX: Disable normalizing ???
	glDisable(GL_NORMALIZE);

	// Enose tin camera me to frustum kai dose times gia tin proboli
	camera.frustum = &frustum;
	camera.fov = 70.0f;
	camera.zFar = 1000.0f;
	camera.zNear = 1.0f;

	/// metaballs initialization
	grid = new C_CubeGrid();
	grid->Constructor(0.0f , 0.0f , -80.0f);

	metaball[0].Constructor();
	metaball[0].position.x = 10.0f;
	metaball[0].position.y = 10.0f;
	metaball[0].position.z = 10.0f;
	metaball[0].radius = 5.0f;

	metaball[1].Constructor();
	metaball[1].position.x = 10.0f;
	metaball[1].position.y = 10.0f;
	metaball[1].position.z = 10.0f;
	metaball[1].radius = 8.0f;

	metaball[2].Constructor();
	metaball[2].position.x = 15.0f;
	metaball[2].position.y = 15.0f;
	metaball[2].position.z = 15.0f;
	metaball[2].radius = 3.0f;

	/// Shaders
	shaderManager = C_GLShaderManager::getSingleton();

	bspShader = shaderManager->LoadShaderProgram("shaders/metaballs_shader.vert", "shaders/metaballs_shader.frag");
   assert(bspShader->verticesAttribLocation >= 0);
   assert(bspShader->normalsAttribLocation >= 0);

#ifdef DRAW_BSP_GEOMETRY
   basicShader = shaderManager->LoadShaderProgram("shaders/wire_shader.vert", "shaders/wire_shader.frag");
   assert(basicShader->verticesAttribLocation >= 0);
   assert(basicShader->normalsAttribLocation == -1);

   pointShader = shaderManager->LoadShaderProgram("shaders/points_shader.vert", "shaders/points_shader.frag");
   assert(pointShader->verticesAttribLocation >= 0);
   assert(pointShader->normalsAttribLocation == -1);
#endif

   wallShader = shaderManager->LoadShaderProgram("shaders/shader1.vert", "shaders/shader1.frag");
   assert(wallShader->verticesAttribLocation >= 0);

   /// Texture manager
   textureManager = C_TextureManager::getSingleton();

   map.createMap("map.txt");

   int tileStartx, tileStarty;
   C_Vertex cameraPosition = map.cameraStartPosition(&tileStartx, &tileStarty);
//   printf("cameraPosition: %f %f %f\n", cameraPosition.x, cameraPosition.y, cameraPosition.z);
   camera.SetPosition(cameraPosition);
   camera.Rotate(0.0f, 180.0f);

   cube.loadFromFile("objmodels/cube.obj");
   cube.shader = wallShader;

   party.setMap(&map);
   party.setCoordinates(tileStartx, tileStarty);

	/// timer initialization
	timer.Initialize ();
}

static void
shutdown(void)
{
   delete shaderManager;

   map.~C_Map();
}

static void
Draw(void)
{
   static float angle = 0.0f;
   static float radius = 4.0f;
   C_Vertex offset;
	/// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera.Look();

   offset.x = radius * cos(angle);
   offset.z = radius * sin(angle);
   offset.y = 0.0f;

	cube.position.x = camera.position.x + offset.x;
	cube.position.y = camera.position.y;
	cube.position.z = camera.position.z + offset.z;

	lightPosition = math::transformPoint(&globalViewMatrix, &cube.position);

	angle += .03f;
	if(angle >= 360.0f) angle = 0.0f;

   party.update();

	cube.draw(&camera);

   map.draw(&camera);

#ifndef JNI_COMPATIBLE
	/// Print text on screem
//	int line = 1;
//	int lineHeight = 18;
//	camera.PrintText(0 , lineHeight * line++ ,
//					 1.0f , 1.0f , 0.0f , 0.6f ,
//					 "FPS: %d" , (int)fps);

	/// Update timer
	timer.Update ();
	timeElapsed = timer.GetDelta () / 1000.0f;

	CountFPS ();

	glutSwapBuffers();
#endif
}


#ifndef JNI_COMPATIBLE
static void
reshape(GLint w , GLint h)
{
	windowWidth = w;
	windowHeight = h;
	camera.setProjection(w , h);
}

static void
idle(void)
{
	glutPostRedisplay();
}

static void
mouse_look(int x , int y)
{
	static int oldX = 0;
	static int oldY = 0;
	float xx = 360 * (windowWidth - x) / windowWidth;
	float yy = 360 * (windowHeight - y) / windowHeight;

	if(oldY != yy) {
		camera.Rotate(2 * (yy - oldY) , 0.0);
		oldY = yy;
	}

	if(oldX != xx) {
		camera.Rotate(0.0 , 2 * (xx - oldX));
		oldX = xx;
	}

	glutPostRedisplay();
}

static void
hande_simple_keys(unsigned char key , int x , int y)
{
	switch(key) {
		case 27 : case 13 :	//ESC
		   shutdown();
			exit(0);
			break;

		case 'w' : case 'W' :
//			camera.Move(TILE_SIZE);
			party.move(MOVE_FORWARD);
			break;

		case 's' : case 'S' :
//			camera.Move(-TILE_SIZE);
			party.move(MOVE_BACKWARDS);
			break;

		case 'a' : case 'A' :
//         camera.Rotate(0.0f, 90.0f);
         party.move(MOVE_TURN_LEFT);
			break;

      case 'd' : case 'D' :
//         camera.Rotate(0.0f, -90.0f);
         party.move(MOVE_TURN_RIGHT);
			break;

		case 'z' : case 'Z' :
			camera.MoveUp(speed);
			break;

		case 'x' : case 'X' :
			camera.MoveDown(speed);
			break;

		case 'q' : case 'Q' :
//			camera.StrafeLeft(TILE_SIZE);
         party.move(MOVE_STRAFE_LEFT);
			break;

		case 'e' : case 'E' :
//			camera.StrafeRight(TILE_SIZE);
         party.move(MOVE_STRAFE_RIGHT);
			break;


//		case 'x' : case 'X' :
//			bspRenderingType = (bspRenderingType + 1) % 4;
//			printf("bspRenderingType: %d\n", bspRenderingType);
//			break;

		default:
			cout << int (key) << '\n';
			break;
   }
}

/// arrow keys handling
static void
handle_arrows(int key , int x , int y)
{
	switch(key) {
   case GLUT_KEY_UP:
      camera.Move(TILE_SIZE);
      break;

   case GLUT_KEY_DOWN:
      camera.Move(-TILE_SIZE);
      break;

   case GLUT_KEY_RIGHT:
			camera.StrafeRight(TILE_SIZE);
//      camera.Rotate(0.0f, -90.0f);
      break;

   case GLUT_KEY_LEFT:
			camera.StrafeLeft(TILE_SIZE);
//      camera.Rotate(0.0f, 90.0f);
      break;
	}

	glutPostRedisplay();
}
#endif

void
CountFPS (void)
{
	static ULONG count = 0.0f;
	float delta = timer.GetTime () - start;
	count++;

	if(delta >= 1000.0f) {
		fps = count;
		start = timer.GetTime ();

		printf("fps: %d\n", count);

		count = 0;
	}
}

int
main(int argc, char* argv[])
{
#ifndef JNI_COMPATIBLE
	glutInit(&argc, argv);

	/// Double buffering with depth buffer
	glutInitDisplayMode(/*GLUT_DOUBLE | */GLUT_RGB | GLUT_DEPTH);

	/// Create window
	glutInitWindowSize(windowWidth , windowHeight);
	glutInitWindowPosition(windowPositionX , windowPositionY);

	winID = glutCreateWindow("...");

	glutReshapeFunc(reshape);
	glutDisplayFunc(Draw);
	glutPassiveMotionFunc(mouse_look);
	glutSpecialFunc(handle_arrows);
	glutKeyboardFunc(hande_simple_keys);
	glutIdleFunc(idle);

	InitGLExtensions();
	CheckGLSL();
	Initializations();

	glutMainLoop();
#else
	CheckGLSL();
	Initializations();
#endif

	return 0;
}
