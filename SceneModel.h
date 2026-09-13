///////////////////////////////////////////////////
//
//	Hamish Carr
//	October, 2023
//
//	------------------------
//	SceneModel.h
//	------------------------
//	
//	The model of the scene
//	
///////////////////////////////////////////////////

#ifndef __SCENE_MODEL_H
#define __SCENE_MODEL_H

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "IndexedFaceSurface.h"
#include "Terrain.h"
#include "Matrix4.h"
#include "Quaternion.h"
#include "BVHData.h"

class SceneModel										
	{ // class SceneModel
	public:	
	// three terrain models
	Terrain flatLandModel;
	Terrain stripeLandModel;
	Terrain rollingLandModel;

	// and a pointer to keep track of the active one
	Terrain *activeLandModel;

	// the view matrix - updated by the interface code
	Matrix4 viewMatrix;

	// the frame number for use in animating
	unsigned long frameNumber;

	float animationTime;

	bool runningTarget;
	float blendWeight;

	std::vector<Cartesian3> blendedPose;

	BVHData standPose;
	BVHData runCycle;

	Cartesian3 characterPosition;

	IndexedFaceSurface ballModel;
	Cartesian3 ballPosition;
	Cartesian3 ballVelocity;

	IndexedFaceSurface dodecahedronModel;
	bool dodecahedronActive;
	Cartesian3 angularVelocity;
	Quaternion orientation;

	bool ballTouchingCharacter;

	int characterHitCount;
	
	// constructor
	SceneModel();

	// routine that updates the scene for the next frame
	void Update();

	// routine to tell the scene to render itself
	void Render();

	// character control events: WASD
	void EventCharacterForward();
	void EventCharacterBackward();

	void ToggleRunning();
	
	// reset game
	void ResetGame();

	// routine to reset the simulation
	void ResetPhysics();

	// routine to switch between flat land and rolling land
	void SwitchLand();
	
	// routine to switch between sphere and dodecahedron
	void SwitchModel();
	
	// routine to rotate launch direction to the left
	void RotateLaunchLeft();
	
	// and to rotate to right
	void RotateLaunchRight();

	void RenderCharacter();

	void RenderBall();

	private:
	void RenderJoint(const Joint &joint, const std::vector<Cartesian3> &rotations);

	void DrawBone(const Cartesian3 &offset);

	GLUquadric *boneQuadric;
	}; // class SceneModel

#endif
	
