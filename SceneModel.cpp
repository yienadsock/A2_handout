///////////////////////////////////////////////////
//
//	Hamish Carr
//	October, 2023
//
//	------------------------
//	SceneModel.cpp
//	------------------------
//	
//	The model of the scene
//
//	
///////////////////////////////////////////////////

#include "SceneModel.h"
#include <math.h>
#include <Quaternion.h>

// three local variables with the hardcoded file names
const char* flatLandModelName		= "./models/flatland.dem";
const char* stripeLandModelName	= "./models/stripeland.dem";
const char* rollingLandModelName	= "./models/rollingland.dem";
const char* sphereModelName		= "./models/spheroid.face";
const char* dodecahedronModelName	= "./models/dodecahedron.face";

// four type of character's animation
const char* motionBvhStand		= "./models/stand.bvh";
const char* motionBvhRun			= "./models/fast_run.bvh";

// the speed of camera movement
const float cameraSpeed = 5.0;

// this is 60 fps nominal speed
const float frameTime = 0.0166667;	

// character configuration
const float characterScale	= 0.01f;	// the BVH skeleton is in roughly centimetre units, world units are metres
const float characterYaw	= 90.0f;	// degrees around the world z axis, so that the character faces right across the screen
const float boneRadius		= 2.0f;		// radius of the bone cylinders, in skeletal units

const Homogeneous4 sunDirection(0.5, -0.5, 0.3, 0.0);
const GLfloat groundColour[4] = { 0.2, 0.5, 0.2, 1.0 };
const GLfloat ballColour[4] = { 0.6, 0.6, 0.6, 1.0 };
const GLfloat characterColour[4] = { 1.0, 1.0, 0.0, 1.0 };
const GLfloat sunAmbient[4] = {0.1, 0.1, 0.1, 1.0 };
const GLfloat sunDiffuse[4] = {0.7, 0.7, 0.7, 1.0 };
const GLfloat blackColour[4] = {0.0, 0.0, 0.0, 1.0};

// constructor
SceneModel::SceneModel()
    { // constructor

    // load landscape models from files
    flatLandModel.ReadFileTerrainData(flatLandModelName, 3);
    stripeLandModel.ReadFileTerrainData(stripeLandModelName, 3);
    rollingLandModel.ReadFileTerrainData(rollingLandModelName, 3);

	// load the character's BVH animation data
	if (!standPose.ReadFileBVH(motionBvhStand))
		std::cout << "Failed to load " << motionBvhStand << std::endl;
	if (!runCycle.ReadFileBVH(motionBvhRun))
		std::cout << "Failed to load " << motionBvhRun << std::endl;

	// sanity check for debugging - report what was loaded
	std::cout << "Loaded " << motionBvhStand << ": "
		<< standPose.all_joints.size() << " joints, "
		<< standPose.frame_count << " frames" << std::endl;
	std::cout << "Loaded " << motionBvhRun << ": "
		<< runCycle.all_joints.size() << " joints, "
		<< runCycle.frame_count << " frames" << std::endl;

	// set the reference for the terrain model to use
    this->activeLandModel = &flatLandModel;
	
	// set the initial view matrix
	viewMatrix = Matrix4::Translate(Cartesian3(0.0, 15.0, -10.0));
	
	// and set the frame number to 0
	frameNumber = 0;
		
	// the quadric used for the bone cylinders is created the first time we render
	boneQuadric = NULL;

	// call the reset routine to initialise the ball position
	ResetPhysics();
	} // constructor

// routine that updates the scene for the next frame
void SceneModel::Update()
	{ // Update()

	} // Update()

// routine to tell the scene to render itself
void SceneModel::Render()
	{ // Render()
	// enable Z-buffering
	glEnable(GL_DEPTH_TEST);
	
	// set lighting parameters
	glShadeModel(GL_FLAT);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, blackColour);
	glLightfv(GL_LIGHT0, GL_EMISSION, blackColour);
	
	// background is sky-blue
	glClearColor(0.7, 0.7, 1.0, 1.0);

	// clear the buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set the modelview matrix
	glMatrixMode(GL_MODELVIEW);
	
	// start with the identity
	glLoadIdentity();

	// add the final rotation from z-up to z-backwords
	glRotatef(-90.0, 1.0, 0.0, 0.0);

	// now compute the view matrix by combining camera translation & rotation
	columnMajorMatrix columnMajorViewMatrix = viewMatrix.columnMajor();
	glMultMatrixf(columnMajorViewMatrix.coordinates);

	// set the light position
	glLightfv(GL_LIGHT0, GL_POSITION, &(sunDirection.x));

	// and set a material colour for the ground
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, groundColour);
	glMaterialfv(GL_FRONT, GL_SPECULAR, blackColour);
	glMaterialfv(GL_FRONT, GL_EMISSION, blackColour);

	// render the terrain
    activeLandModel->Render();

	// render the character's skeleton on top of it
	RenderCharacter();

    } // Render()

// routine to render the character's skeleton
void SceneModel::RenderCharacter()
	{ // RenderCharacter()
	// draw the character in its own colour
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, characterColour);
	glMaterialfv(GL_FRONT, GL_SPECULAR, blackColour);
	glMaterialfv(GL_FRONT, GL_EMISSION, blackColour);

	// build the transform from skeletal space into world space
	glPushMatrix();

	// move the character to its position in the world
	glTranslatef(characterPosition.x, characterPosition.y, characterPosition.z);

	// yaw the character so that it faces right across the screen
	glRotatef(characterYaw, 0.0f, 0.0f, 1.0f);

	// the world is z-up, but the BVH skeleton is y-up - stand the character upright
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

	// scale the skeleton from skeletal units into world units
	glScalef(characterScale, characterScale, characterScale);

	// render the hierarchy, starting from the root joint
	RenderJoint(standPose.root);

	glPopMatrix();
	} // RenderCharacter()

// routine to recursively render a joint and the bones leading to its children
void SceneModel::RenderJoint(const Joint &joint)
	{ // RenderJoint()
	glPushMatrix();

	// move to this joint's position relative to its parent
	glTranslatef(joint.joint_offset[0], joint.joint_offset[1], joint.joint_offset[2]);

	// (Task Ib will add the joint's animated rotation here)

	// draw a bone to each child joint, then recurse down the hierarchy
	for (int child = 0; child < (int) joint.Children.size(); child++)
		{ // per child
		DrawBone(Cartesian3(	joint.Children[child].joint_offset[0],
								joint.Children[child].joint_offset[1],
								joint.Children[child].joint_offset[2]));
		RenderJoint(joint.Children[child]);
		} // per child

	glPopMatrix();
	} // RenderJoint()

// routine to render a single bone as a cylinder from the origin to the given offset
void SceneModel::DrawBone(const Cartesian3 &offset)
	{ // DrawBone()
	// work out the length of the bone and ignore degenerate bones
	float length = offset.length();
	if (length < 1e-6f)
		return;

	// create the quadric used for the cylinders the first time we draw a bone
	if (boneQuadric == NULL)
		{ // create quadric
		boneQuadric = gluNewQuadric();
		gluQuadricDrawStyle(boneQuadric, GLU_FILL);
		gluQuadricNormals(boneQuadric, GLU_SMOOTH);
		} // create quadric

	// a cylinder is generated along the +z axis, so work out the rotation that aligns it with the bone
	Cartesian3 direction = offset / length;
	Cartesian3 axis = Cartesian3(0.0f, 0.0f, 1.0f).cross(direction);
	float sinAngle = axis.length();
	float cosAngle = direction.z;

	glPushMatrix();
	if (sinAngle < 1e-6f)
		{ // bone is parallel to the z axis
		// if it points backwards, flip the cylinder over, otherwise leave it alone
		if (cosAngle < 0.0f)
			glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
		} // bone is parallel to the z axis
	else
		{ // general case
		float angle = atan2(sinAngle, cosAngle) * 180.0f / M_PI;
		glRotatef(angle, axis.x, axis.y, axis.z);
		} // general case

	// draw the shaft of the bone
	gluCylinder(boneQuadric, boneRadius, boneRadius, length, 8, 1);

	glPopMatrix();
	} // DrawBone()

// character control events: W for forward
void SceneModel::EventCharacterForward()
    { // EventCharacterForward()

    } // EventCharacterForward()

// character control events: S for backward
void SceneModel::EventCharacterBackward()
    { // EventCharacterBackward()

    } // EventCharacterBackward()

void SceneModel::ResetGame()
    { // ResetGame()
    this->ResetPhysics();
    } // ResetGame()

// routine to reset the simulation
void SceneModel::ResetPhysics()
	{ // ResetPhysics()
	std::cout << "Resetting Physics." << std::endl;
	} // ResetPhysics()
	
// routine to switch between flat land and rolling land
void SceneModel::SwitchLand()
	{ // SwitchLand()
	// toggle between terrains
	if (activeLandModel == &flatLandModel)
		activeLandModel = &stripeLandModel;
	else if (activeLandModel == &stripeLandModel)
		activeLandModel = &rollingLandModel;
	else if (activeLandModel == &rollingLandModel)
		activeLandModel = &flatLandModel;
	} // SwitchLand()
	
// routine to switch between sphere and dodecahedron
void SceneModel::SwitchModel()
	{ // SwitchModel()
	// and reset the physics
	ResetPhysics();
	} // SwitchModel()
