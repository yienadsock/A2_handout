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

	// the character's animation clock (seconds)
	float animationTime;

	// start/stop request & blend between rest (0) and run (1)
	bool runningTarget;
	float blendWeight;

	// store pose for blending rest & run
	std::vector<Cartesian3> blendedPose;

	// character animation data: rest pose & run cycle
	BVHData standPose;
	BVHData runCycle;

	// character position in the world (z-up)
	Cartesian3 characterPosition;

	// the ball's surface model & position in the world (z-up)
	IndexedFaceSurface ballModel;
	Cartesian3 ballPosition;
	
	// constructor
	SceneModel();

	// routine that updates the scene for the next frame
	void Update();

	// routine to tell the scene to render itself
	void Render();

	// character control events: WASD
	void EventCharacterForward();
	void EventCharacterBackward();

	// start or stop the character running
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

	// render the character's skeleton
	void RenderCharacter();

	// render the ball
	void RenderBall();

	private:
	// recursively draw a joint and its child bones in the given pose
	void RenderJoint(const Joint &joint, const std::vector<Cartesian3> &rotations);

	// draw one bone as a cylinder from the origin to the given offset
	void DrawBone(const Cartesian3 &offset);

	// quadric used for the bone cylinders (created on first render)
	GLUquadric *boneQuadric;
	}; // class SceneModel

#endif
	
