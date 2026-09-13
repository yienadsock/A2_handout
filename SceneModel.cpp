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

const float characterScale	= 0.01f;
const float characterYaw	= 90.0f;
const float characterSpeed	= 4.0f;
const float blendDuration	= 0.5f;
const float boneRadius		= 2.0f;

const float gravity		= 9.8f;
const float elasticity	= 0.6f;
const float ballRadius	= 1.0f;

const float dodecahedronInertia	= 0.4f;
const float restingSpeed		= 0.5f;

const float characterHeight	= 1.8f;
const float characterRadius	= 0.3f;

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

	if (!ballModel.ReadFileIndexedFace(sphereModelName))
		std::cout << "Failed to load " << sphereModelName << std::endl;

	if (!dodecahedronModel.ReadFileIndexedFace(dodecahedronModelName))
		std::cout << "Failed to load " << dodecahedronModelName << std::endl;

	if (!standPose.ReadFileBVH(motionBvhStand))
		std::cout << "Failed to load " << motionBvhStand << std::endl;
	if (!runCycle.ReadFileBVH(motionBvhRun))
		std::cout << "Failed to load " << motionBvhRun << std::endl;

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
		
	animationTime = 0.0f;

	runningTarget = false;
	blendWeight = 0.0f;
	blendedPose.resize(standPose.boneRotations[0].size());

	dodecahedronActive = false;

	boneQuadric = NULL;

	// call the reset routine to initialise the ball position
	ResetPhysics();
	} // constructor

// routine that updates the scene for the next frame
void SceneModel::Update()
	{ // Update()
	animationTime += frameTime;

	blendWeight += (runningTarget ? frameTime : -frameTime) / blendDuration;
	if (blendWeight < 0.0f) blendWeight = 0.0f;
	if (blendWeight > 1.0f) blendWeight = 1.0f;

	characterPosition.x += characterSpeed * blendWeight * frameTime;

	characterPosition.z = activeLandModel->getHeight(characterPosition.x, characterPosition.y);

	ballVelocity.z -= gravity * frameTime;
	ballPosition = ballPosition + ballVelocity * frameTime;

	if (dodecahedronActive)
		{
		int contactVertex = -1;
		float deepest = 0.0f;
		Cartesian3 contactNormal;
		Cartesian3 contactOffset;
		for (int vertex = 0; vertex < (int) dodecahedronModel.vertices.size(); vertex++)
			{
			Cartesian3 offset = orientation.Conjugate().Act(dodecahedronModel.vertices[vertex]);
			Cartesian3 point = ballPosition + offset;
			float penetration = activeLandModel->getHeight(point.x, point.y) - point.z;
			if (penetration > deepest)
				{
				deepest = penetration;
				contactVertex = vertex;
				contactOffset = offset;
				contactNormal = activeLandModel->getNormal(point.x, point.y);
				}
			}
		if (contactVertex >= 0)
			{
			ballPosition.z += deepest;
			Cartesian3 contactVelocity = ballVelocity + angularVelocity.cross(contactOffset);
			float vn = contactVelocity.dot(contactNormal);
			if (vn < 0.0f)
				{
				float restitution = elasticity;
				if (vn > -restingSpeed)
					restitution = 0.0f;
				Cartesian3 lever = contactOffset.cross(contactNormal);
				float impulse = -(1.0f + restitution) * vn / (1.0f + lever.dot(lever) / dodecahedronInertia);
				ballVelocity = ballVelocity + contactNormal * impulse;
				angularVelocity = angularVelocity + lever * (impulse / dodecahedronInertia);
				}
			}

		Quaternion spin(angularVelocity.x, angularVelocity.y, angularVelocity.z, 0.0f);
		orientation = (orientation + (spin * orientation) * (0.5f * frameTime)).Unit();
		}
	else
		{
		float groundHeight = activeLandModel->getHeight(ballPosition.x, ballPosition.y);
		if (ballPosition.z - groundHeight < ballRadius)
			{
			ballPosition.z = groundHeight + ballRadius;
			Cartesian3 normal = activeLandModel->getNormal(ballPosition.x, ballPosition.y);
			float vn = ballVelocity.dot(normal);
			if (vn < 0.0f)
				ballVelocity = ballVelocity - normal * ((1.0f + elasticity) * vn);
			}
		}

	Cartesian3 characterCentre(characterPosition.x, characterPosition.y, characterPosition.z + 0.5f * characterHeight);
	bool ballTouching = (ballPosition - characterCentre).length() < ballRadius + characterRadius;
	if (ballTouching && !ballTouchingCharacter)
		{
		characterHitCount++;
		std::cout << "Ball hit the character! hits: " << characterHitCount << std::endl;
		}
	ballTouchingCharacter = ballTouching;
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

	RenderBall();

	RenderCharacter();

    } // Render()

void SceneModel::RenderBall()
	{
	//ball's material
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ballColour);
	glMaterialfv(GL_FRONT, GL_SPECULAR, blackColour);
	glMaterialfv(GL_FRONT, GL_EMISSION, blackColour);

	glPushMatrix();
	glTranslatef(ballPosition.x, ballPosition.y, ballPosition.z);
	if (dodecahedronActive)
		{
		columnMajorMatrix orientationMatrix = orientation.GetMatrix().columnMajor();
		glMultMatrixf(orientationMatrix.coordinates);
		dodecahedronModel.Render();
		}
	else
		ballModel.Render();
	glPopMatrix();
	}

void SceneModel::RenderCharacter()
	{
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, characterColour);
	glMaterialfv(GL_FRONT, GL_SPECULAR, blackColour);
	glMaterialfv(GL_FRONT, GL_EMISSION, blackColour);

	glPushMatrix();

	glTranslatef(characterPosition.x, characterPosition.y, characterPosition.z);

	glRotatef(characterYaw, 0.0f, 0.0f, 1.0f);

	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

	glScalef(characterScale, characterScale, characterScale);

	if (!runCycle.boneRotations.empty() && runCycle.frame_time > 0.0f)
		{
		size_t frame = ((size_t) (animationTime / runCycle.frame_time)) % runCycle.boneRotations.size();
		const std::vector<Cartesian3> &runPose = runCycle.boneRotations[frame];

		if (blendWeight <= 0.0f)
			RenderJoint(runCycle.root, standPose.boneRotations[0]);
		else if (blendWeight >= 1.0f)
			RenderJoint(runCycle.root, runPose);
		else
			{
			const std::vector<Cartesian3> &restPose = standPose.boneRotations[0];
			for (size_t joint = 0; joint < runPose.size(); joint++)
				blendedPose[joint] = restPose[joint] + (runPose[joint] - restPose[joint]) * blendWeight;
			RenderJoint(runCycle.root, blendedPose);
			}
		}

	glPopMatrix();
	}

void SceneModel::RenderJoint(const Joint &joint, const std::vector<Cartesian3> &rotations)
	{
	glPushMatrix();

	glTranslatef(joint.joint_offset[0], joint.joint_offset[1], joint.joint_offset[2]);

	const Cartesian3 &angles = rotations[joint.id];
	glRotatef(angles.x, 1.0f, 0.0f, 0.0f);
	glRotatef(angles.y, 0.0f, 1.0f, 0.0f);
	glRotatef(angles.z, 0.0f, 0.0f, 1.0f);

	for (int child = 0; child < (int) joint.Children.size(); child++)
		{
		DrawBone(Cartesian3(	joint.Children[child].joint_offset[0],
								joint.Children[child].joint_offset[1],
								joint.Children[child].joint_offset[2]));
		RenderJoint(joint.Children[child], rotations);
		}

	glPopMatrix();
	}

void SceneModel::DrawBone(const Cartesian3 &offset)
	{
	float length = offset.length();
	if (length < 1e-6f)
		return;

	if (boneQuadric == NULL)
		{
		boneQuadric = gluNewQuadric();
		gluQuadricDrawStyle(boneQuadric, GLU_FILL);
		gluQuadricNormals(boneQuadric, GLU_SMOOTH);
		}

	Cartesian3 direction = offset / length;
	Cartesian3 axis = Cartesian3(0.0f, 0.0f, 1.0f).cross(direction);
	float sinAngle = axis.length();
	float cosAngle = direction.z;

	glPushMatrix();
	if (sinAngle < 1e-6f)
		{
		if (cosAngle < 0.0f)
			glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
		}
	else
		{
		float angle = atan2(sinAngle, cosAngle) * 180.0f / M_PI;
		glRotatef(angle, axis.x, axis.y, axis.z);
		}

	gluCylinder(boneQuadric, boneRadius, boneRadius, length, 8, 1);

	glPopMatrix();
	}

// character control events: W for forward
void SceneModel::EventCharacterForward()
    { // EventCharacterForward()

    } // EventCharacterForward()

// character control events: S for backward
void SceneModel::EventCharacterBackward()
    { // EventCharacterBackward()

    } // EventCharacterBackward()

void SceneModel::ToggleRunning()
	{
	runningTarget = !runningTarget;
	}

void SceneModel::ResetGame()
    { // ResetGame()
    this->ResetPhysics();
    } // ResetGame()

// routine to reset the simulation
void SceneModel::ResetPhysics()
	{ // ResetPhysics()
	std::cout << "Resetting Physics." << std::endl;

	ballPosition = Cartesian3(0.0, 0.0, 10.0);
	ballVelocity = Cartesian3(0.0, 0.0, 0.0);

	angularVelocity = Cartesian3(0.0, 0.0, 0.0);
	orientation = Quaternion();

	ballTouchingCharacter = false;
	characterHitCount = 0;
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
	dodecahedronActive = !dodecahedronActive;

	// and reset the physics
	ResetPhysics();
	} // SwitchModel()
