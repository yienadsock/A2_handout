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
const float characterScale	= 0.01f;	// skeletal units (cm) to world metres
const float characterYaw	= 90.0f;	// the character faces screen-right
const float characterSpeed	= 4.0f;		// forward run speed (m/s)
const float blendDuration	= 0.5f;		// rest <-> run blend time (seconds)
const float boneRadius		= 2.0f;		// bone cylinder radius, in skeletal units

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

	// load the ball model
	if (!ballModel.ReadFileIndexedFace(sphereModelName))
		std::cout << "Failed to load " << sphereModelName << std::endl;

	// load the character's animation data
	if (!standPose.ReadFileBVH(motionBvhStand))
		std::cout << "Failed to load " << motionBvhStand << std::endl;
	if (!runCycle.ReadFileBVH(motionBvhRun))
		std::cout << "Failed to load " << motionBvhRun << std::endl;

	// report what was loaded
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
		
	// start the animation clock
	animationTime = 0.0f;

	// start in the rest pose
	runningTarget = false;
	blendWeight = 0.0f;
	blendedPose.resize(standPose.boneRotations[0].size());

	// the bone quadric is created on the first render
	boneQuadric = NULL;

	// call the reset routine to initialise the ball position
	ResetPhysics();
	} // constructor

// routine that updates the scene for the next frame
void SceneModel::Update()
	{ // Update()
	// advance the animation clock
	animationTime += frameTime;

	// blend between rest & run
	blendWeight += (runningTarget ? frameTime : -frameTime) / blendDuration;
	if (blendWeight < 0.0f) blendWeight = 0.0f;
	if (blendWeight > 1.0f) blendWeight = 1.0f;

	// run forward (the character faces +x), eased by the blend
	characterPosition.x += characterSpeed * blendWeight * frameTime;

	// follow the terrain height
	characterPosition.z = activeLandModel->getHeight(characterPosition.x, characterPosition.y);
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

	// render the ball
	RenderBall();

	// render the character
	RenderCharacter();

    } // Render()

// render the ball
void SceneModel::RenderBall()
	{ 
	//ball's material
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ballColour);
	glMaterialfv(GL_FRONT, GL_SPECULAR, blackColour);
	glMaterialfv(GL_FRONT, GL_EMISSION, blackColour);

	// move the model to the ball's position
	glPushMatrix();
	glTranslatef(ballPosition.x, ballPosition.y, ballPosition.z);
	ballModel.Render();
	glPopMatrix();
	} 

// render the character's skeleton
void SceneModel::RenderCharacter()
	{ // RenderCharacter()
	// set the character's material
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, characterColour);
	glMaterialfv(GL_FRONT, GL_SPECULAR, blackColour);
	glMaterialfv(GL_FRONT, GL_EMISSION, blackColour);

	// transform skeletal space into world space
	glPushMatrix();

	// world position
	glTranslatef(characterPosition.x, characterPosition.y, characterPosition.z);

	// face screen-right
	glRotatef(characterYaw, 0.0f, 0.0f, 1.0f);

	// BVH is y-up, world is z-up
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

	// skeletal units to metres
	glScalef(characterScale, characterScale, characterScale);

	// current frame of the run cycle
	if (!runCycle.boneRotations.empty() && runCycle.frame_time > 0.0f)
		{ // has animation data
		size_t frame = ((size_t) (animationTime / runCycle.frame_time)) % runCycle.boneRotations.size();
		const std::vector<Cartesian3> &runPose = runCycle.boneRotations[frame];

		// draw the rest pose, the run pose, or a blend of the two
		if (blendWeight <= 0.0f)
			RenderJoint(runCycle.root, standPose.boneRotations[0]);
		else if (blendWeight >= 1.0f)
			RenderJoint(runCycle.root, runPose);
		else
			{ // blend the poses
			const std::vector<Cartesian3> &restPose = standPose.boneRotations[0];
			for (size_t joint = 0; joint < runPose.size(); joint++)
				blendedPose[joint] = restPose[joint] + (runPose[joint] - restPose[joint]) * blendWeight;
			RenderJoint(runCycle.root, blendedPose);
			} // blend the poses
		} // has animation data

	glPopMatrix();
	} // RenderCharacter()

// recursively draw a joint and the bones to its children
void SceneModel::RenderJoint(const Joint &joint, const std::vector<Cartesian3> &rotations)
	{ // RenderJoint()
	glPushMatrix();

	// move to the joint's offset from its parent
	glTranslatef(joint.joint_offset[0], joint.joint_offset[1], joint.joint_offset[2]);

	// apply the joint's rotation
	// (rotating X then Y then Z gives RX * RY * RZ, as pdf specifies)
	const Cartesian3 &angles = rotations[joint.id];
	glRotatef(angles.x, 1.0f, 0.0f, 0.0f);
	glRotatef(angles.y, 0.0f, 1.0f, 0.0f);
	glRotatef(angles.z, 0.0f, 0.0f, 1.0f);

	// draw a bone to each child, then recurse
	for (int child = 0; child < (int) joint.Children.size(); child++)
		{ // per child
		DrawBone(Cartesian3(	joint.Children[child].joint_offset[0],
								joint.Children[child].joint_offset[1],
								joint.Children[child].joint_offset[2]));
		RenderJoint(joint.Children[child], rotations);
		} // per child

	glPopMatrix();
	} // RenderJoint()

// draw one bone as a cylinder from the origin to the given offset
void SceneModel::DrawBone(const Cartesian3 &offset)
	{ // DrawBone()
	// skip degenerate bones
	float length = offset.length();
	if (length < 1e-6f)
		return;

	// create the quadric on first use
	if (boneQuadric == NULL)
		{ // create quadric
		boneQuadric = gluNewQuadric();
		gluQuadricDrawStyle(boneQuadric, GLU_FILL);
		gluQuadricNormals(boneQuadric, GLU_SMOOTH);
		} // create quadric

	// rotate the cylinder's +z axis onto the bone direction
	Cartesian3 direction = offset / length;
	Cartesian3 axis = Cartesian3(0.0f, 0.0f, 1.0f).cross(direction);
	float sinAngle = axis.length();
	float cosAngle = direction.z;

	glPushMatrix();
	if (sinAngle < 1e-6f)
		{ // parallel to the z axis
		// flip if the bone points backwards
		if (cosAngle < 0.0f)
			glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
		} // parallel to the z axis
	else
		{ // general case
		float angle = atan2(sinAngle, cosAngle) * 180.0f / M_PI;
		glRotatef(angle, axis.x, axis.y, axis.z);
		} // general case

	// draw the bone
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

// start or stop the character running
void SceneModel::ToggleRunning()
	{ // ToggleRunning()
	runningTarget = !runningTarget;
	} // ToggleRunning()

void SceneModel::ResetGame()
    { // ResetGame()
    this->ResetPhysics();
    } // ResetGame()

// routine to reset the simulation
void SceneModel::ResetPhysics()
	{ // ResetPhysics()
	std::cout << "Resetting Physics." << std::endl;

	// drop the ball from 10 m
	ballPosition = Cartesian3(0.0, 0.0, 10.0);
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
