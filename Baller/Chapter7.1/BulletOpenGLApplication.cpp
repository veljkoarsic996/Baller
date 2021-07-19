#include "BulletOpenGLApplication.h"
#include <iostream>
#include <fmod.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Some constants for 3D math and the camera speed
#define RADIANS_PER_DEGREE 0.01745329f
#define CAMERA_STEP_SIZE 3.5f
#define WM_MOUSEMOVE 0x0200


//variables for game
//textures
const int texturesSize = 1;
GLuint textures[texturesSize];
char* textureFiles[] = {
	"resources/portal.jpg"
};
float z = 0;

//audio
FMOD::System* audiomgr;
FMOD::Sound* musBackground;
FMOD::Sound* sfxDeath;
FMOD::Sound* sfxJump;
FMOD::Sound* sfxPickUp;
FMOD::Sound* sfxJumpAvailable;
FMOD::Sound* sfxDenied;
FMOD::Sound* sfxVictory;
//
int jumpCount = 0;
int jumpLimit = 1;
GLfloat speed = 20.0f;
bool grounded;
bool pauseFlag = false;
bool startFlag = true;
bool endFlag = false;



char* pauseTxt = " ";

btVector3 objTxtColor = { 1,0,0 };
char* objectiveTxt = " ";
btVector3 endTxtColor = { 1,0,0 };
char* endTxt = " ";
char* jumpTxt = " ";



btTransform startPos;

GLfloat angle = 0.0f;

GLuint startingLives = 3;
GLuint lives = startingLives;

GLuint activeRemainingKeys = 0;
GLuint remainingKeys = 0;
GameObject* player;
GameObject* portal;

btVector3 currentVector;
btVector3 playerPos = { 0, 10, -5 };
btVector3 velocity = { 0,0,0 };
btVector3 cameraPos;
btVector3 upVector;
btVector3 jumpVector;
btVector3 cPos; 

btVector3 portalColor;
btVector3 portalPos;

float jumpRestartTimer;
float jumpRestartTreshold;
float deltaTime = 0.01f;


BulletOpenGLApplication::BulletOpenGLApplication() 
:
m_cameraPosition(10.0f, 5.0f, 0.0f),
m_cameraTarget(0.0f, 0.0f, 0.0f),
m_cameraDistance(15.0f),
m_cameraPitch(20.0f),
m_cameraYaw(0.0f),
m_upVector(0.0f, 1.0f, 0.0f),
m_nearPlane(1.0f),
m_farPlane(1000.0f),
m_pBroadphase(0),
m_pCollisionConfiguration(0),
m_pDispatcher(0),
m_pSolver(0),
m_pWorld(0),
m_pPickedBody(0),
m_pPickConstraint(0)
{
}

BulletOpenGLApplication::~BulletOpenGLApplication() {
	// shutdown the physics system
	ShutdownPhysics();
}


void BulletOpenGLApplication::loadTexture(char* filename, GLuint id) {

	int width, height, channels;

	unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);
}

bool InitFmod()
{
	FMOD_RESULT result;
	result = FMOD::System_Create(&audiomgr);
	if (result != FMOD_OK)
	{
		return false;
	}
	result = audiomgr->init(50, FMOD_INIT_NORMAL, NULL);
	if (result != FMOD_OK)
	{
		return false;
	}
	return true;
}

const bool LoadAudio()
{
	FMOD_RESULT result;
	result = audiomgr->createSound("resources/jump.wav", FMOD_DEFAULT, 0, &sfxJump);
	result = audiomgr->createSound("resources/jumpAvailable.wav", FMOD_DEFAULT, 0, &sfxJumpAvailable);
	result = audiomgr->createSound("resources/death.wav", FMOD_DEFAULT, 0, &sfxDeath);
	result = audiomgr->createSound("resources/pickUp.wav", FMOD_DEFAULT, 0, &sfxPickUp);	
	result = audiomgr->createSound("resources/victory.mp3", FMOD_DEFAULT, 0, &sfxVictory);	
	result = audiomgr->createSound("resources/denied.wav", FMOD_DEFAULT, 0, &sfxDenied);

	result = audiomgr->createSound("resources/background.mp3", FMOD_LOOP_NORMAL | FMOD_2D, 0, &musBackground);
	FMOD::Channel* channel;
	result = audiomgr->playSound(musBackground, 0, false, &channel);
	return true;
}                                          


void BulletOpenGLApplication::DrawStrokeText(char* string, int x, int y, const float r, const float g, const float b) // Stroke Font
{
	char* c;

	glColor3f(r, g, b);
	glPushMatrix();
	glTranslatef(x, y + 8, 0.0);
	glLineWidth(2.0);
	glScalef(0.14f, -0.12f, 0.0);

	for (c = string; *c != '\0'; c++)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
	}
	glPopMatrix();

	glColor3f(1.0f, 1.0f, 1.0f);
}

void BulletOpenGLApplication::DrawText(char* string, float x, float y, const float r, const float g, const float b)  // BItmap Font
{
	glColor3f(r, g, b);
	glLineWidth(4.0);
	char* c;
	glRasterPos3f(x, y, 0.0);
	for (c = string; *c != '\0'; c++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
	}

	glColor3f(1.0f, 1.0f, 1.0f);
}


bool redKeyFlag = false;
bool blueKeyFlag = false;
bool cyanKeyFlag = false;
bool purpleKeyFlag = false;

void BulletOpenGLApplication::Initialize() {
	// this function is called inside glutmain() after
	// creating the window, but before handing control
	// to FreeGLUT
	InitFmod();
	LoadAudio();
	// create some floats for our ambient, diffuse, specular and position
	GLfloat ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // dark grey
	GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // white
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // white
	GLfloat position[] = { 5.0f, 10.0f, 1.0f, 0.0f };
	
	// set the ambient, diffuse, specular and position for LIGHT0
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glEnable(GL_LIGHTING); // enables lighting
	glEnable(GL_LIGHT0); // enables the 0th light
	glEnable(GL_COLOR_MATERIAL); // colors materials when lighting is enabled
		
	// enable specular lighting via materials
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMateriali(GL_FRONT, GL_SHININESS, 15);
	
	// enable smooth shading
	glShadeModel(GL_SMOOTH);
	
	// enable depth testing to be 'less than'
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// set the backbuffer clearing color to a lightish blue
	glClearColor(0.6, 0.65, 0.85, 0);

	// initialize the physics system
	InitializePhysics();

	// create the debug drawer
	m_pDebugDrawer = new DebugDrawer();
	// set the initial debug level to 0
	m_pDebugDrawer->setDebugMode(0);
	// add the debug drawer to the world
	m_pWorld->setDebugDrawer(m_pDebugDrawer);

	player = CreateGameObject(new btSphereShape(1), 1, btVector3(0, 0, 1), playerPos, btQuaternion(1, 0, 0));

	


	InitGround();
	portalColor = {0,0,0};
	createPortal();
	createKeys();
	activeRemainingKeys = remainingKeys;

	jumpRestartTreshold = 3.0f;
	jumpRestartTimer = 0.0f;

	glEnable(GL_TEXTURE_2D);
	glGenTextures(texturesSize, textures);
	loadTexture(textureFiles[0], textures[0]);
}

void BulletOpenGLApplication::restartPos() {
	player->GetRigidBody()->setActivationState(ACTIVE_TAG);
	player->GetRigidBody()->setDeactivationTime(0.0f);
	startPos.setOrigin(btVector3(0, 10, -5));
	player->GetRigidBody()->setWorldTransform(startPos);
	player->GetRigidBody()->setLinearVelocity({ 0,0,0 });
	m_cameraPitch = 20.0f;
	m_cameraYaw = 0.0f;
	grounded = true;
	jumpCount = 0;
}

void BulletOpenGLApplication::restartGame() {
	player->GetRigidBody()->setActivationState(ACTIVE_TAG);
	player->GetRigidBody()->setDeactivationTime(0.0f);
	grounded = true;
	jumpCount = 0;
	player->GetRigidBody()->setLinearVelocity({ 0,0,0 });
	m_cameraPitch = 20.0f;
	m_cameraYaw = 0.0f;

	 z = 0;
	 pauseFlag = false;
	 startFlag = true;
	 endFlag = false;

	 pauseTxt = " ";
	 objTxtColor = { 1,0,0 };
	 objectiveTxt = " ";
	 endTxtColor = { 1,0,0 };
	 endTxt = " ";
	 endTxt = " ";
	 angle = 0.0f;
	 lives = 3;
}
//keys
GameObject* redKey;
GameObject* blueKey;
GameObject* cyanKey;
GameObject* purpleKey;
GameObject* yellowKey;


void BulletOpenGLApplication::createPortal() {
	portal = makePortal(new btBoxShape(btVector3({ 4,4,0.5 })), 0, portalColor, btVector3({ 0,4,0 }), btQuaternion(0, 0, 0));
	GameObject* portalFrame1 = CreateGameObject(new btBoxShape(btVector3({ 0.5,4,1 })), 0, btVector3(0, 0, 0), btVector3({ 4.5,4,0 }), btQuaternion(0, 0, 0));
	GameObject* portalFrame2 = CreateGameObject(new btBoxShape(btVector3({ 0.5,4,1 })), 0, btVector3(0, 0, 0), btVector3({ -4.5,4,0 }), btQuaternion(0, 0, 0));
	GameObject* portalFrame3 = CreateGameObject(new btBoxShape(btVector3({ 5,0.2,1 })), 0, btVector3(0, 0, 0), btVector3({ 0,8.2,0 }), btQuaternion(0, 0, 0));
}
void BulletOpenGLApplication::InitGround() {

	GameObject* startGround;
	startGround = CreateGroundObject(new btBoxShape(btVector3(1, 20, 20)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(0, 0, 0));
	GameObject* ground2;
	ground2 = CreateGroundObject(new btBoxShape(btVector3(1, 20, 20)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(45, -0.5, 0));
	GameObject* groundRed;
	groundRed = CreateGroundObject(new btBoxShape(btVector3(1, 20, 20)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(50, -1, -45));
	groundRed->SetColor({1,1,1});

}
void BulletOpenGLApplication::checkKeyFlags() {
	if (redKeyFlag == true) {
		GameObject* Obsticle1;
		Obsticle1 = CreateGroundObject(new btBoxShape(btVector3(1, 30, 0.8)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-15, -4, -60));

		GameObject* groundBlue;
		groundBlue = CreateGroundObject(new btBoxShape(btVector3(1, 10, 20)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-60, -3, -60));
		redKeyFlag = false;
		groundBlue->SetColor({ 1,1,1 });
	}	
	if (blueKeyFlag == true) {
		GameObject* platform1;
		platform1 = CreateGroundObject(new btBoxShape(btVector3(1, 2, 2)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-80, -2, -60));
		GameObject* platform2;
		platform2 = CreateGroundObject(new btBoxShape(btVector3(1, 2, 2)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-90, -2, -70));
		GameObject* platform4;
		platform4 = CreateGroundObject(new btBoxShape(btVector3(1, 2, 2)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-105, -2, -60));


		GameObject* groundCyan;
		groundCyan = CreateGroundObject(new btBoxShape(btVector3(1, 10, 10)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-125, -1.5, -50));
		groundCyan->SetColor({ 1,1,1 });
		blueKeyFlag = false;
	}

	if (cyanKeyFlag == true) {
		GameObject* ground1;
		ground1 = CreateGroundObject(new btBoxShape(btVector3(1, 1, 10)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-125, -1.5, -20));
		GameObject* ground2;
		ground2 = CreateGroundObject(new btBoxShape(btVector3(5, 0.2, 1.5)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-125, 0, -5));
		GameObject* ground3;
		ground3 = CreateGroundObject(new btBoxShape(btVector3(1, 1, 10)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-125, -1.5, 10));

		GameObject* purpleGround; 
		purpleGround = CreateGroundObject(new btBoxShape(btVector3(1, 10, 10)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-125, -1.5, 40));
		purpleGround->SetColor({ 1,1,1 });
		cyanKeyFlag = false;
	}
	if (purpleKeyFlag == true) {
		GameObject* yellowGround;
		yellowGround = CreateGroundObject(new btBoxShape(btVector3(1, 1, 1)), 0, btVector3(0.0f, 1.0f, 0.0f), btVector3(-125, -1.5, 60));
		yellowGround->SetColor({ 1,1,1 });


		purpleKeyFlag = false;
	}
	
}


void BulletOpenGLApplication::restartJump(float p_deltaTime) {

	if (grounded == false) {
		jumpRestartTimer += p_deltaTime;
		   if (jumpRestartTimer > jumpRestartTreshold) {
			   grounded = true;
			   jumpCount = 0;
			   jumpTxt = "JUMP AVAILABLE";
			   FMOD::Channel* channel;
			   audiomgr->playSound(sfxJumpAvailable, 0, false, &channel);
		   }
		   if (jumpRestartTimer > 1) {
			  	 jumpVector[1] = -1;
				 jumpVector.normalize();
				 jumpVector *= 8 ;
		   }
	}
}
void BulletOpenGLApplication::playerJump() {
	jumpVector[1] = 1;
	jumpVector.normalize();
	jumpVector *= 8;
	jumpCount++;
	player->GetRigidBody()->setLinearVelocity(jumpVector);
}


btVector3 move;
void BulletOpenGLApplication::movePlayerRight() {
	btVector3 right = (m_cameraTarget - m_cameraPosition).cross({ 0,1,0 });
	move = {right[0],jumpVector[1],right[2]};
	move.normalize();
	move *= speed;
	player->GetRigidBody()->setLinearVelocity(move);

}
void BulletOpenGLApplication::movePlayerLeft() {
	btVector3 left = (m_cameraTarget - m_cameraPosition).cross({0,-1,0});
	move = { left[0],jumpVector[1],left[2] };
	move.normalize();
	move *= speed;
	player->GetRigidBody()->setLinearVelocity(move);
}
void BulletOpenGLApplication::movePlayerBack() {
	btVector3 backwards = m_cameraPosition - m_cameraTarget;
	move = { backwards[0],jumpVector[1],backwards[2] };
	move.normalize();
	move *= speed;
	player->GetRigidBody()->setLinearVelocity(move);
}
void BulletOpenGLApplication::movePlayerForward() {
	btVector3 forward = m_cameraTarget - m_cameraPosition;
	move = {forward[0],jumpVector[1],forward[2]};
	move.normalize();
	move *= speed;
	player->GetRigidBody()->setLinearVelocity(move);
}



void BulletOpenGLApplication::Keyboard(unsigned char key, int x, int y) {
	// This function is called by FreeGLUT whenever
	// generic keys are pressed down.

	if (key == 'p' || key == 'P') {
		if (pauseFlag == true) {
			pauseFlag = false;
		}
		else if (pauseFlag == false) {
			ShowCursor(TRUE);
			pauseFlag = true;
		}
	}

	if (key == 'r' || key == 'R') {
		if (endFlag == true) {
			restartKeys();
			restartGame();
			restartPos();
		}
	}
}

void BulletOpenGLApplication::KeyboardUp(unsigned char key, int x, int y) {
	// This function is called by FreeGLUT whenever
	// generic keys are pressed down.
	if (key == 'w' || key == 'W') {
		velocity = { 0,-1,0 };
		velocity.normalize();
		velocity *= 0.1f;
		// set the linear velocity of the box
		player->GetRigidBody()->setLinearVelocity(velocity);
	}
	if (key == ' ') {
		if (startFlag == true) {
			ShowCursor(FALSE);
			startFlag = false;
			pauseFlag = false;
		}
	}
}
//key flags
void BulletOpenGLApplication::createKeys() {
	redKey = makeKey(new btBoxShape(btVector3(0.5, 0.2, 0.5)), 0, btVector3(1.0f, 0.0f, 0.0f), btVector3(50, 1, -60));
	redKey->SetColor({ 1,0,0 }); 

	blueKey = makeKey(new btBoxShape(btVector3(0.5, 0.2, 0.5)), 0, btVector3(0.0f, 0.0f, 1.0f), btVector3(-60, -0.5, -60));
	blueKey->SetColor({ 0,0,1 });

	cyanKey = makeKey(new btBoxShape(btVector3(0.5, 0.2, 0.5)), 0, btVector3(0.5f, 1.0f, 1.0f), btVector3(-125, 0.5, -50));
	cyanKey->SetColor({0.5f, 1.0f, 1.0f});

	purpleKey = makeKey(new btBoxShape(btVector3(0.5, 0.2, 0.5)), 0, btVector3(1.0f, 0.5f, 1.0f), btVector3(-125, 0.5, 40));
	purpleKey->SetColor({ 0.7f, 0.0f, 1.0f });

	yellowKey = makeKey(new btBoxShape(btVector3(0.5, 0.2, 0.5)), 0, btVector3(1.0f, 1.0f, 0.0f), btVector3(-125, 0.5, 60));
	yellowKey->SetColor({ 1.0f, 1.0f, 0.0f });
}

void BulletOpenGLApplication::RenderKeys() {
	btScalar transform[16];
	for (GameObjects::iterator i = keys.begin(); i != keys.end(); ++i) {
		// get the object from the iterator
		GameObject* pObj = *i;

		// read the transform
		pObj->GetTransform(transform);

		// get data from the object and draw it
		DrawShape(transform, pObj->GetShape(), pObj->GetColor());
	}
}

void BulletOpenGLApplication::destroyKeyObject(btRigidBody* pBody) {
	for (GameObjects::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
		if ((*iter)->GetRigidBody() == pBody) {
			GameObject* pObject = *iter;
			// remove the rigid body from the world
			// erase the object from the list
			keys.erase(iter);
			// delete the object from memory
			//delete pBody;
			// done
			return;
		}
	}
}

void BulletOpenGLApplication::restartKeys() {
	for (GameObjects::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
		GameObject* pObject = (*iter);
		// remove the rigid body from the world
		m_pWorld->removeRigidBody(pObject->GetRigidBody());
		// erase the object from the list
		keys.clear();
		//delete the object from memory
		//delete pObject;
		return;
	}
	remainingKeys = 0;
	createKeys();
	activeRemainingKeys = remainingKeys;
}

void BulletOpenGLApplication::checkForKeyCollision() {
	btVector3 keyPos;

		for (auto obj : keys) {
		keyPos = obj->GetRigidBody()->getWorldTransform().getOrigin();
		if (playerPos[2] <= keyPos[2] + 1 && playerPos[2] >= keyPos[2] - 1) {
			if (playerPos[0] <= keyPos[0] + 1 && playerPos[0] >= keyPos[0] - 1) {
				destroyKeyObject(obj->GetRigidBody());
				//std::cout << "radi";
				activeRemainingKeys--;
				//portal->SetColor(obj->GetColor());
				FMOD::Channel* channel;
				audiomgr->playSound(sfxPickUp, 0, false, &channel);
				if (obj == redKey) {
					redKeyFlag = true;
				}
				if (obj == blueKey) {
					blueKeyFlag = true;
				}
				if (obj == cyanKey) {
					cyanKeyFlag = true;
				}
				if (obj == purpleKey) {
					purpleKeyFlag = true;
				}
			}
		}
	}
	}

void BulletOpenGLApplication::checkForPortal() {
	portalPos = portal->GetRigidBody()->getWorldTransform().getOrigin() ;
	if (playerPos[2] <= portalPos[2]+1 && playerPos[2] >= portalPos[2]-1) {
		if (playerPos[0] <= portalPos[0] + 5 && playerPos[0] >= portalPos[0] - 5) {
			if (0 != activeRemainingKeys) {
				FMOD::Channel* channel;
				audiomgr->playSound(sfxDenied, 0, false, &channel);
				objTxtColor = { 1.0,0.0,0.0 };
				objectiveTxt = "Niste sakupili sve kljuceve!";
			}
			else if (0 == activeRemainingKeys) {
				FMOD::Channel* channel;
				audiomgr->playSound(sfxVictory, 0, false, &channel);
				player->GetRigidBody()->setLinearVelocity({ 0,0,0 });
				objTxtColor = { 0.1,1.0,0.1 };
				objectiveTxt = "CESTITAM! Presli ste igricu!";
				endTxt = "Kliknite R za restart";
				endFlag = true;
			}

			}
				else {
					objTxtColor = { 0, 0,0 };
					objectiveTxt = " ";
				}

		}
	else {

		objTxtColor = { 0, 0,0 };
		objectiveTxt = " ";
	}
	
	}




void BulletOpenGLApplication::UpdateGame() {
	// exit in erroneous situations
	if (m_screenWidth == 0 && m_screenHeight == 0)
		return;



	checkForPortal();

	// select the projection matrix
	glMatrixMode(GL_PROJECTION);
	// set it to the matrix-equivalent of 1
	glLoadIdentity();
	// determine the aspect ratio of the screen
	float aspectRatio = m_screenWidth / (float)m_screenHeight;
	// create a viewing frustum based on the aspect ratio and the
	// boundaries of the camera
	glFrustum(-aspectRatio * m_nearPlane, aspectRatio * m_nearPlane, -m_nearPlane, m_nearPlane, m_nearPlane, m_farPlane);
	// the projection matrix is now set

	// select the view matrix
	glMatrixMode(GL_MODELVIEW);
	// set it to '1'
	glLoadIdentity();

	m_pWorld->removeRigidBody(portal->GetRigidBody());

	// our values represent the angles in degrees, but 3D 
	// ath typically demands angular values are in radians.
	float pitch = m_cameraPitch * RADIANS_PER_DEGREE;
	float yaw = m_cameraYaw * RADIANS_PER_DEGREE;

	// create a quaternion defining the angular rotation 
	// around the up vector
	btQuaternion rotation(m_upVector, yaw);

	// set the camera's position to 0,0,0, then move the 'z' 
	// position to the current value of m_cameraDistance.

	btVector3 cameraPosition(0, 0, 0);
	cameraPosition[2] = -m_cameraDistance;

	// create a Bullet Vector3 to represent the camera 
	// position and scale it up if its value is too small.
	btVector3 forward(cameraPosition[0], cameraPosition[1], cameraPosition[2]);
	if (forward.length2() < SIMD_EPSILON) {
		forward.setValue(1.f, 0.f, 0.f);
	}

	// figure out the 'right' vector by using the cross 
	// product on the 'forward' and 'up' vectors
	btVector3 right = m_upVector.cross(forward);

	// create a quaternion that represents the camera's roll
	btQuaternion roll(right, -pitch);

	// turn the rotation (around the Y-axis) and roll (around 
	// the forward axis) into transformation matrices and 
	// apply them to the camera position. This gives us the 
	// final position
	cameraPosition = btMatrix3x3(rotation) * btMatrix3x3(roll) * cameraPosition;
	// save our new position in the member variable, and 
	// shift it relative to the target position (so that we 
	// orbit it)

	playerPos = player->GetRigidBody()->getWorldTransform().getOrigin();

	m_cameraTarget[0] = playerPos.getX();
	m_cameraTarget[1] = playerPos.getY();
	m_cameraTarget[2] = playerPos.getZ();

	m_cameraPosition[0] = cameraPosition.getX();
	m_cameraPosition[1] = cameraPosition.getY();
	m_cameraPosition[2] = cameraPosition.getZ();
	m_cameraPosition += m_cameraTarget;

	if (grounded == false) {
		player->GetRigidBody()->forceActivationState(ACTIVE_TAG);
	}

	if (grounded == true) {
		jumpCount = 0;
	}

	if (lives == 0) {
		FMOD::Channel* channel;
		audiomgr->playSound(sfxDeath, 0, false, &channel);
		restartKeys();
		lives = startingLives;
		player->GetRigidBody()->setActivationState(false);
		player->GetRigidBody()->setDeactivationTime(0.0f);
		objTxtColor = { 1,0,0 };
		endTxtColor = { 1,0,0 };
		objectiveTxt = "Izgubili ste sve zivote!";
		endTxt = "Kliknite R da restartujete ili ESC za izlaz";
		objTxtColor = { 0.1,1.0,0.1 };
		endFlag = true;
	}

	if (lives != 0 && playerPos[1] < -3) {
		FMOD::Channel* channel;
		audiomgr->playSound(sfxDeath, 0, false, &channel);
		lives--;
		if (lives != 0) {
			restartPos();
			jumpRestartTimer = 0.0f;
		}

	}

	if (endFlag == true) {
		player->GetRigidBody()->setActivationState(false);
		player->GetRigidBody()->setDeactivationTime(0.0f);
	}

	if (startFlag == true && endFlag == false) {
		endFlag = false;
		player->GetRigidBody()->setActivationState(false);
		player->GetRigidBody()->setDeactivationTime(0.0f);
		pauseTxt = "Press SPACE to start";
	}
	else if (pauseFlag == true && startFlag == false && endFlag == false) {
		endFlag = false;
		player->GetRigidBody()->setActivationState(false);
		player->GetRigidBody()->setDeactivationTime(0.0f);
		pauseTxt = "PAUSED";
	}
	else if (pauseFlag == false && startFlag == false && endFlag == false) {
		endFlag = false;
		player->GetRigidBody()->setActivationState(ACTIVE_TAG);
		player->GetRigidBody()->setDeactivationTime(0.0f);
		pauseTxt = " ";
	}

	if (activeRemainingKeys == 0) {
		lives = 1;
		z = 1;
	}


	checkKeyFlags();
	restartJump(deltaTime);

	btVector3 originalPos = { -150, -1.5, 70 };
	btVector3 purplePos = { -150, -1.5, 35 };
	btVector3 yellow = { -100, -1.5, 70 };



	// create a view matrix based on the camera's position and where it's
	// looking
	gluLookAt(m_cameraPosition[0] , 10 , m_cameraPosition[2], m_cameraTarget[0], 1, m_cameraTarget[2],  m_upVector.getX(), m_upVector.getY(), m_upVector.getZ());
	// the view matrix is now set

}




void BulletOpenGLApplication::Special(int key, int x, int y) {

}

void BulletOpenGLApplication::SpecialUp(int key, int x, int y) {}

void BulletOpenGLApplication::Reshape(int w, int h) {
	// this function is called once during application intialization
	// and again every time we resize the window

	// grab the screen width/height
	m_screenWidth = w;
	m_screenHeight = h;
	glutReshapeWindow(800, 600);
	// set the viewport
	glViewport(0, 0, w, h);
	// update the camera
	UpdateGame();
}
void BulletOpenGLApplication::Idle() {
	// this function is called frequently, whenever FreeGlut
	// isn't busy processing its own events. It should be used
	// to perform any updating and rendering tasks

	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glDisable(GL_LIGHTING);
	glPushMatrix();
	int grid = 3;
	float ones[] = { 1, 1, 1, 1 };
	float zeros[] = { 0, 0, 0, 1 };
	float half[] = { 0.5, 0.5, 0.5, 1 };

	// all shiny
	glMaterialfv(GL_FRONT, GL_SPECULAR, ones);
	glMaterialf(GL_FRONT, GL_SHININESS, 50);
	// get the time since the last iteration
	float dt = m_clock.getTimeMilliseconds();
	// reset the clock to 0
	m_clock.reset();
	// update the scene (convert ms to s)
	UpdateScene(dt / 1000.0f);

	// update the camera
	UpdateGame();

	// render the scene
	RenderKeys();
	RenderScene();

	drawHud();

	glPopMatrix();
	// swap the front and back buffers
	glutSwapBuffers();
}

void BulletOpenGLApplication::Mouse(int button, int state, int x, int y) {
}

void BulletOpenGLApplication::PassiveMotion(int x, int y) {
}

int lastX;
int lastY;
void BulletOpenGLApplication::Motion(int x, int y) {

	//POINT p;
	//if (GetCursorPos(&p))
		if (lastX > x) {
			RotateCamera(m_cameraYaw, -CAMERA_STEP_SIZE);
		}if (lastX < x) {
			RotateCamera(m_cameraYaw, +CAMERA_STEP_SIZE);
		}
		lastX = x;

}

void BulletOpenGLApplication::handleInput() {



	if (GetKeyState(0x20) & 0x8000) {
		if (grounded == true && startFlag == false && pauseFlag == false && endFlag == false) {
			//std::cout << jumpCount << std::endl;
			if (jumpCount < jumpLimit) {
				FMOD::Channel* channel;
				audiomgr->playSound(sfxJump, 0, false, &channel);
				jumpTxt = " ";
				grounded = false;
				jumpRestartTimer = 0.0f;
				playerJump();
			}
		}
		player->GetRigidBody()->forceActivationState(ACTIVE_TAG);
	
	}

	if (GetKeyState(VK_ESCAPE) & 0x8100) {
		exit(0);
	}
	if (GetKeyState(0x57) & 0x8000) {
		if (pauseFlag == false) {
			movePlayerForward();
			player->GetRigidBody()->forceActivationState(ACTIVE_TAG);
		}
	}
	
	 if (GetKeyState(0x53) & 0x8000) {
		if (pauseFlag == false) {
			movePlayerBack();
			player->GetRigidBody()->forceActivationState(ACTIVE_TAG);
		}
	}
	 if (GetKeyState(0x41) & 0x8000) {
		 if (pauseFlag == false && startFlag == false) {
			 player->GetRigidBody()->forceActivationState(ACTIVE_TAG);
			 movePlayerLeft();
		 }
	}
	 if (GetKeyState(0x44) & 0x8000) {
		 if (pauseFlag == false && startFlag == false) {
			 player->GetRigidBody()->forceActivationState(ACTIVE_TAG);
			 movePlayerRight();
		 }
	}
}
void BulletOpenGLApplication::Display() {
	handleInput();
}
void BulletOpenGLApplication::drawPortalWithTexture(const btVector3& halfSize) {


		glColor3ub(155, 0, 255);
	float halfWidth = halfSize.x();
	float halfHeight = halfSize.y();
	float halfDepth = halfSize.z();
	float d = 4;

	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glBegin(GL_QUADS);

	// front
	glNormal3f(0, 0, 1);
	glTexCoord2f(z, z);
	glVertex3f(halfWidth, -halfHeight, halfDepth);
	glTexCoord2f(z, 0);
	glVertex3f(halfWidth, halfHeight, halfDepth);
	glTexCoord2f(0, 0);
	glVertex3f(-halfWidth, halfHeight, halfDepth);
	glTexCoord2f(0, z);
	glVertex3f(-halfWidth, -halfHeight, halfDepth);
	// back
	glNormal3f(0, 0, 1);
	glTexCoord2f(z, z);
	glVertex3f(halfWidth, -halfHeight, -halfDepth - 0.1);
	glTexCoord2f(z, 0);
	glVertex3f(halfWidth, halfHeight, -halfDepth - 0.1);
	glTexCoord2f(0, 0);
	glVertex3f(-halfWidth, halfHeight, -halfDepth - 0.1);
	glTexCoord2f(0, z);
	glVertex3f(-halfWidth, -halfHeight, -halfDepth - 0.1);

	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
}

void BulletOpenGLApplication::DrawBox(const btVector3 &halfSize) {
	
	float halfWidth = halfSize.x();
	float halfHeight = halfSize.y();
	float halfDepth = halfSize.z();

	// create the vertex positions
	btVector3 vertices[8]={	
	btVector3(halfWidth,halfHeight,halfDepth),
	btVector3(-halfWidth,halfHeight,halfDepth),
	btVector3(halfWidth,-halfHeight,halfDepth),	
	btVector3(-halfWidth,-halfHeight,halfDepth),	
	btVector3(halfWidth,halfHeight,-halfDepth),
	btVector3(-halfWidth,halfHeight,-halfDepth),	
	btVector3(halfWidth,-halfHeight,-halfDepth),	
	btVector3(-halfWidth,-halfHeight,-halfDepth)};

	// create the indexes for each triangle, using the 
	// vertices above. Make it static so we don't waste 
	// processing time recreating it over and over again
	static int indices[36] = {
		0,1,2,
		3,2,1,
		4,0,6,
		6,0,2,
		5,1,4,
		4,1,0,
		7,3,1,
		7,1,5,
		5,4,7,
		7,4,6,
		7,2,3,
		7,6,2};

	// start processing vertices as triangles
	glBegin (GL_TRIANGLES);

	// increment the loop by 3 each time since we create a 
	// triangle with 3 vertices at a time.

	for (int i = 0; i < 36; i += 3) {
		// get the three vertices for the triangle based
		// on the index values set above
		// use const references so we don't copy the object
		// (a good rule of thumb is to never allocate/deallocate
		// memory during *every* render/update call. This should 
		// only happen sporadically)
		const btVector3 &vert1 = vertices[indices[i]];
		const btVector3 &vert2 = vertices[indices[i+1]];
		const btVector3 &vert3 = vertices[indices[i+2]];

		// create a normal that is perpendicular to the 
		// face (use the cross product)
		btVector3 normal = (vert3-vert1).cross(vert2-vert1);
		normal.normalize ();

		// set the normal for the subsequent vertices
		glNormal3f(normal.getX(),normal.getY(),normal.getZ());

		// create the vertices
		glVertex3f (vert1.x(), vert1.y(), vert1.z());
		glVertex3f (vert2.x(), vert2.y(), vert2.z());
		glVertex3f (vert3.x(), vert3.y(), vert3.z());
	}

	// stop processing vertices
	glEnd();
}


void BulletOpenGLApplication::RotateCamera(float& angle, float value) {// change the value (it is passed by reference, so we
	// can edit it here)
	angle -= value;
	// keep the value within bounds
	if (angle < 0) angle += 360;
	if (angle >= 360) angle -= 360;
	// update the camera since we changed the angular value
	UpdateGame();
}

void BulletOpenGLApplication::drawHud() {
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-5,300,300,0,0,1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();


	char life[50];
	sprintf_s(life, 50, "Lives: %i", lives);
	DrawText(life,1.0f, 20.0f, 1.0f, 0.0f, 0.0f);

	char keyss[50];
	sprintf_s(keyss, 50, "Remaining Keys: %i", activeRemainingKeys);
	DrawText(keyss, 1.0f, 35.0f, 1.0f, 1.0f, 1.0f);

	DrawText(pauseTxt, 120.0f, 20.0f, 1.0f, 0.0f, 0.0f);
	DrawText(jumpTxt, 1.0f, 50.0f, 0.0f, 1.0f, 0.0f);

	DrawText(objectiveTxt, 120.0f, 35.0f , objTxtColor[0], objTxtColor[1], objTxtColor[2]);
	DrawText(endTxt, 120.0f, 45.0f, endTxtColor[0], endTxtColor[1], endTxtColor[2]);


	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
	
}
void BulletOpenGLApplication::DrawPortal(btScalar* transform, const btCollisionShape* pShape) {
	glBindTexture(GL_TEXTURE_2D,textures[0]);


	// push the matrix stack
	glPushMatrix();
	glMultMatrixf(transform);
		const btBoxShape* box = static_cast<const btBoxShape*>(pShape);
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		drawPortalWithTexture(halfSize);
	glPopMatrix();

}

void BulletOpenGLApplication::RenderScene() {

	// create an array of 16 floats (representing a 4x4 matrix)
	btScalar transform[16];

	glPushMatrix();
	float ones[] = { 1, 1, 1, 1 };
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ones);
	portal->GetTransform(transform);
	DrawPortal(transform,portal->GetShape());
	glPopMatrix();

	// iterate through all of the objects in our world
	for(GameObjects::iterator i = m_objects.begin(); i != m_objects.end(); ++i) {
		// get the object from the iterator
		GameObject* pObj = *i;

		// read the transform
		pObj->GetTransform(transform);

		// get data from the object and draw it
		DrawShape(transform, pObj->GetShape(), pObj->GetColor());
	}
	for (GameObjects::iterator i = groundObjects.begin(); i != groundObjects.end(); ++i) {
		// get the object from the iterator
		GameObject* pObj = *i;

		// read the transform
		pObj->GetTransform(transform);

		// get data from the object and draw it
		DrawShape(transform, pObj->GetShape(), pObj->GetColor());
	}

	// after rendering all game objects, perform debug rendering
	// Bullet will figure out what needs to be drawn then call to
	// our DebugDrawer class to do the rendering for us
	m_pWorld->debugDrawWorld();




}


void BulletOpenGLApplication::UpdateScene(float dt) {
	// check if the world object exists
	if (m_pWorld) {
		// step the simulation through time. This is called
		// every update and the amount of elasped time was 
		// determined back in ::Idle() by our clock object.
		m_pWorld->stepSimulation(dt);

		// check for any new collisions/separations
		CheckForCollisionEvents();
		checkForKeyCollision();
	}
	
}

void BulletOpenGLApplication::DrawShape(btScalar* transform, const btCollisionShape* pShape, const btVector3 &color) {
	// set the color
	glColor3f(color.x(), color.y(), color.z());

	// push the matrix stack
	glPushMatrix();
	glMultMatrixf(transform);

	// make a different draw call based on the object type
	switch(pShape->getShapeType()) {
		// an internal enum used by Bullet for boxes
	case BOX_SHAPE_PROXYTYPE:
		{
			// assume the shape is a box, and typecast it
			const btBoxShape* box = static_cast<const btBoxShape*>(pShape);
			// get the 'halfSize' of the box
			btVector3 halfSize = box->getHalfExtentsWithMargin();
			// draw the box
			DrawBox(halfSize);
			break;
		}

/*ADD*/		case SPHERE_SHAPE_PROXYTYPE:
/*ADD*/			{
/*ADD*/				// assume the shape is a sphere and typecast it
/*ADD*/				const btSphereShape* sphere = static_cast<const btSphereShape*>(pShape);
/*ADD*/				// get the sphere's size from the shape
/*ADD*/				float radius = sphere->getMargin();
/*ADD*/				// draw the sphere
/*ADD*/				DrawSphere(radius);
/*ADD*/				break;
/*ADD*/			}
/*ADD*/	
/*ADD*/		case CYLINDER_SHAPE_PROXYTYPE:
/*ADD*/			{
/*ADD*/				// assume the object is a cylinder
/*ADD*/				const btCylinderShape* pCylinder = static_cast<const btCylinderShape*>(pShape);
/*ADD*/				// get the relevant data
/*ADD*/				float radius = pCylinder->getRadius();
/*ADD*/				float halfHeight = pCylinder->getHalfExtentsWithMargin()[1];
/*ADD*/				// draw the cylinder
/*ADD*/				DrawCylinder(radius,halfHeight);
/*ADD*/	
/*ADD*/				break;
/*ADD*/			}

	default:
		// unsupported type
		break;
	}

	// pop the stack
	glPopMatrix();
}

GameObject* BulletOpenGLApplication::makeKey(btCollisionShape* pShape, const float& mass, const btVector3& color, const btVector3& initialPosition) {
	remainingKeys++;
	GameObject* pObject = new GameObject(pShape, mass, color, initialPosition);

	// push it to the back of the list
	keys.push_back(pObject);

	return pObject;

}
GameObject* BulletOpenGLApplication::CreateGroundObject(btCollisionShape* pShape, const float& mass, const btVector3& color, const btVector3& initialPosition) {

	GameObject* pObject = new GameObject(pShape, mass, color, initialPosition);

	// push it to the back of the list
	groundObjects.push_back(pObject);

	// check if the world object is valid
	if (m_pWorld) {
		// add the object's rigid body to the world
		m_pWorld->addRigidBody(pObject->GetRigidBody());
	}
	return pObject;
}
GameObject* BulletOpenGLApplication::makePortal(btCollisionShape* pShape, const float& mass, const btVector3& color, const btVector3& initialPosition, const btQuaternion& initialRotation){
	 portal = new GameObject(pShape, mass, color, initialPosition, initialRotation);

	// push it to the back of the list
	m_objects.push_back(portal);

	// check if the world object is valid
	return portal;
}

GameObject* BulletOpenGLApplication::CreateGameObject(btCollisionShape* pShape, const float &mass, const btVector3 &color, const btVector3 &initialPosition, const btQuaternion &initialRotation) {
	// create a new game object
	GameObject* pObject = new GameObject(pShape, mass, color, initialPosition, initialRotation);

	// push it to the back of the list
	m_objects.push_back(pObject);

	// check if the world object is valid
	if (m_pWorld) {
		// add the object's rigid body to the world
		m_pWorld->addRigidBody(pObject->GetRigidBody());
	}
	return pObject;
}

btVector3 BulletOpenGLApplication::GetPickingRay(int x, int y) {
 	// calculate the field-of-view
 	float tanFov = 1.0f / m_nearPlane;
 	float fov = btScalar(2.0) * btAtan(tanFov);
 	
 	// get a ray pointing forward from the 
 	// camera and extend it to the far plane
 	btVector3 rayFrom = m_cameraPosition;
 	btVector3 rayForward = (m_cameraTarget - m_cameraPosition);
 	rayForward.normalize();
 	rayForward*= m_farPlane;
 	
 	// find the horizontal and vertical vectors 
 	// relative to the current camera view
 	btVector3 ver = m_upVector;
 	btVector3 hor = rayForward.cross(ver);
 	hor.normalize();
 	ver = hor.cross(rayForward);
 	ver.normalize();
 	hor *= 2.f * m_farPlane * tanFov;
 	ver *= 2.f * m_farPlane * tanFov;
 	
 	// calculate the aspect ratio
 	btScalar aspect = m_screenWidth / (btScalar)m_screenHeight;
 	
 	// adjust the forward-ray based on
 	// the X/Y coordinates that were clicked
 	hor*=aspect;
 	btVector3 rayToCenter = rayFrom + rayForward;
 	btVector3 dHor = hor * 1.f/float(m_screenWidth);
 	btVector3 dVert = ver * 1.f/float(m_screenHeight);
 	btVector3 rayTo = rayToCenter - 0.5f * hor + 0.5f * ver;
 	rayTo += btScalar(x) * dHor;
 	rayTo -= btScalar(y) * dVert;
 	
 	// return the final result
 	return rayTo;
}


 	
bool BulletOpenGLApplication::Raycast(const btVector3 &startPosition, const btVector3 &direction, RayResult &output, bool includeStatic) {
 	if (!m_pWorld) 
 		return false;
 		
 	// get the picking ray from where we clicked
 	btVector3 rayTo = direction;
 	btVector3 rayFrom = m_cameraPosition;
 		
 	// create our raycast callback object
 	btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom,rayTo);
 		
 	// perform the raycast
 	m_pWorld->rayTest(rayFrom,rayTo,rayCallback);
 		
 	// did we hit something?
 	if (rayCallback.hasHit())
 	{
 		// if so, get the rigid body we hit
 		btRigidBody* pBody = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);
 		if (!pBody)
 			return false;
 		
 		// prevent us from picking objects 
 		// like the ground plane
		if (!includeStatic) // skip this check if we want it to hit static objects
			if (pBody->isStaticObject() || pBody->isKinematicObject()) 
				return false;
 	    
 		// set the result data
 		output.pBody = pBody;
 		output.hitPoint = rayCallback.m_hitPointWorld;
 		return true;
 	}
 	
 	// we didn't hit anything
 	return false;
}
 	


void BulletOpenGLApplication::DestroyGameObject(btRigidBody* pBody) {
 	// we need to search through the objects in order to 
 	// find the corresponding iterator (can only erase from 
 	// an std::vector by passing an iterator)
 	for (GameObjects::iterator iter = m_objects.begin(); iter != m_objects.end(); ++iter) {
 		if ((*iter)->GetRigidBody() == pBody) {
 			GameObject* pObject = *iter;
 			// remove the rigid body from the world
 			m_pWorld->removeRigidBody(pObject->GetRigidBody());
 			// erase the object from the list
 			m_objects.erase(iter);
 			// delete the object from memory
 			delete pObject;
 			// done
 			return;
 		}
 	}
	for (GameObjects::iterator iter = groundObjects.begin(); iter != groundObjects.end(); ++iter) {
		if ((*iter)->GetRigidBody() == pBody) {
			GameObject* pObject = *iter;
			// remove the rigid body from the world
			m_pWorld->removeRigidBody(pObject->GetRigidBody());
			// erase the object from the list
			groundObjects.erase(iter);
			// delete the object from memory
			delete pObject;
			// done
			return;
		}
	}

}





void BulletOpenGLApplication::CheckForCollisionEvents() {
	// keep a list of the collision pairs we
	// found during the current update
	CollisionPairs pairsThisUpdate;

	// iterate through all of the manifolds in the dispatcher
	for (int i = 0; i < m_pDispatcher->getNumManifolds(); ++i) {

		// get the manifold
		btPersistentManifold* pManifold = m_pDispatcher->getManifoldByIndexInternal(i);

		// ignore manifolds that have 
		// no contact points.
		if (pManifold->getNumContacts() > 0) {
			// get the two rigid bodies involved in the collision
			const btRigidBody* pBody0 = static_cast<const btRigidBody*>(pManifold->getBody0());
			const btRigidBody* pBody1 = static_cast<const btRigidBody*>(pManifold->getBody1());

			// always create the pair in a predictable order
			// (use the pointer value..)
			bool const swapped = pBody0 > pBody1;
			const btRigidBody* pSortedBodyA = swapped ? pBody1 : pBody0;
			const btRigidBody* pSortedBodyB = swapped ? pBody0 : pBody1;

			// create the pair
			CollisionPair thisPair = std::make_pair(pSortedBodyA, pSortedBodyB);

			// insert the pair into the current list
			pairsThisUpdate.insert(thisPair);

			// if this pair doesn't exist in the list
			// from the previous update, it is a new
			// pair and we must send a collision event
			if (m_pairsLastUpdate.find(thisPair) == m_pairsLastUpdate.end()) {
				CollisionEvent((btRigidBody*)pBody0, (btRigidBody*)pBody1);
			}

			for (auto obj : groundObjects) {
				bool const swap = player > obj;
				const btRigidBody* groundBodyA = swap ? obj->GetRigidBody() : player->GetRigidBody();
				const btRigidBody* groundBodyB = swap ? player->GetRigidBody() : obj->GetRigidBody();
				CollisionPair groundPair = std::make_pair(groundBodyA, groundBodyB);

				if (m_pairsLastUpdate.find(groundPair) != m_pairsLastUpdate.end()) {
					CollisionEvent((btRigidBody*)player, (btRigidBody*)obj);
				}
			}

		}
	}
		// create another list for pairs that
		// were removed this update
		CollisionPairs removedPairs;

		// this handy function gets the difference beween
		// two sets. It takes the difference between
		// collision pairs from the last update, and this 
		// update and pushes them into the removed pairs list
		std::set_difference(m_pairsLastUpdate.begin(), m_pairsLastUpdate.end(),
			pairsThisUpdate.begin(), pairsThisUpdate.end(),
			std::inserter(removedPairs, removedPairs.begin()));

		// iterate through all of the removed pairs
		// sending separation events for them
		for (CollisionPairs::const_iterator iter = removedPairs.begin(); iter != removedPairs.end(); ++iter) {
			SeparationEvent((btRigidBody*)iter->first, (btRigidBody*)iter->second);
		}

		// in the next iteration we'll want to
		// compare against the pairs we found
		// in this iteration
		m_pairsLastUpdate = pairsThisUpdate;
	}

	
void BulletOpenGLApplication::CollisionEvent(btRigidBody * pBody0, btRigidBody * pBody1) {

}

void BulletOpenGLApplication::SeparationEvent(btRigidBody * pBody0, btRigidBody * pBody1) {

}

GameObject* BulletOpenGLApplication::FindGameObject(btRigidBody* pBody) {
	// search through our list of gameobjects finding
	// the one with a rigid body that matches the given one
	for (GameObjects::iterator iter = m_objects.begin(); iter != m_objects.end(); ++iter) {
		if ((*iter)->GetRigidBody() == pBody) {
			// found the body, so return the corresponding game object
			return *iter;
		}
	}	
	for (GameObjects::iterator iter = groundObjects.begin(); iter != groundObjects.end(); ++iter) {
		if ((*iter)->GetRigidBody() == pBody) {
			// found the body, so return the corresponding game object
			return *iter;
		}
	}
	for (GameObjects::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
		if ((*iter)->GetRigidBody() == pBody) {
			// found the body, so return the corresponding game object
			return *iter;
		}
	}
	return 0;
}

/*ADD*/	void BulletOpenGLApplication::DrawSphere(const btScalar &radius) {
/*ADD*/		// some constant values for more many segments to build the sphere from
/*ADD*/		static int lateralSegments = 25;
/*ADD*/		static int longitudinalSegments = 25;
/*ADD*/	
/*ADD*/		// iterate laterally
/*ADD*/		for(int i = 0; i <= lateralSegments; i++) {
/*ADD*/			// do a little math to find the angles of this segment
/*ADD*/			btScalar lat0 = SIMD_PI * (-btScalar(0.5) + (btScalar) (i - 1) / lateralSegments);
/*ADD*/			btScalar z0  = radius*sin(lat0);
/*ADD*/			btScalar zr0 =  radius*cos(lat0);
/*ADD*/	
/*ADD*/			btScalar lat1 = SIMD_PI * (-btScalar(0.5) + (btScalar) i / lateralSegments);
/*ADD*/			btScalar z1 = radius*sin(lat1);
/*ADD*/			btScalar zr1 = radius*cos(lat1);
/*ADD*/	
/*ADD*/			// start rendering strips of quads (polygons with 4 poins)
/*ADD*/			glBegin(GL_QUAD_STRIP);
/*ADD*/			// iterate longitudinally
/*ADD*/			for(int j = 0; j <= longitudinalSegments; j++) {
/*ADD*/				// determine the points of the quad from the lateral angles
/*ADD*/				btScalar lng = 2 * SIMD_PI * (btScalar) (j - 1) / longitudinalSegments;
/*ADD*/				btScalar x = cos(lng);
/*ADD*/				btScalar y = sin(lng);
/*ADD*/				// draw the normals and vertices for each point in the quad
/*ADD*/				// since it is a STRIP of quads, we only need to add two points
/*ADD*/				// each time to create a whole new quad
/*ADD*/				
/*ADD*/				// calculate the normal
/*ADD*/				btVector3 normal = btVector3(x*zr0, y*zr0, z0);
/*ADD*/				normal.normalize();
/*ADD*/				glNormal3f(normal.x(), normal.y(), normal.z());
/*ADD*/				// create the first vertex
/*ADD*/				glVertex3f(x * zr0, y * zr0, z0);
/*ADD*/				
/*ADD*/				// calculate the next normal
/*ADD*/				normal = btVector3(x*zr1, y*zr1, z1);
/*ADD*/				normal.normalize();
/*ADD*/				glNormal3f(normal.x(), normal.y(), normal.z());
/*ADD*/				// create the second vertex
/*ADD*/				glVertex3f(x * zr1, y * zr1, z1);
/*ADD*/				
/*ADD*/				// and repeat...
/*ADD*/			}
/*ADD*/			glEnd();
/*ADD*/		}
/*ADD*/	}

/*ADD*/	void BulletOpenGLApplication::DrawCylinder(const btScalar& radius, const btScalar& halfHeight) {
	/*ADD*/		static int slices = 15;
	/*ADD*/		static int stacks = 10;
	/*ADD*/		// tweak the starting position of the
	/*ADD*/		// cylinder to match the physics object
	/*ADD*/		glRotatef(-90.0, 1.0, 0.0, 0.0);
	/*ADD*/		glTranslatef(0.0, 0.0, -halfHeight);
	/*ADD*/		// create a quadric object to render with
	/*ADD*/		GLUquadricObj* quadObj = gluNewQuadric();
	/*ADD*/		// set the draw style of the quadric
	/*ADD*/		gluQuadricDrawStyle(quadObj, (GLenum)GLU_FILL);
	/*ADD*/		gluQuadricNormals(quadObj, (GLenum)GLU_SMOOTH);
	/*ADD*/		// create a disk to cap the cylinder
	/*ADD*/		gluDisk(quadObj, 0, radius, slices, stacks);
	/*ADD*/		// create the main hull of the cylinder (no caps)
	/*ADD*/		gluCylinder(quadObj, radius, radius, 2.f * halfHeight, slices, stacks);
	/*ADD*/		// shift the position and rotation again
	/*ADD*/		glTranslatef(0.0f, 0.0f, 2.f * halfHeight);
	/*ADD*/		glRotatef(-180.0f, 0.0f, 1.0f, 0.0f);
	/*ADD*/		// draw the cap on the other end of the cylinder
	/*ADD*/		gluDisk(quadObj, 0, radius, slices, stacks);
	/*ADD*/		// don't need the quadric anymore, so remove it
	/*ADD*/		// to save memory
	/*ADD*/		gluDeleteQuadric(quadObj);
}
/*ADD*/