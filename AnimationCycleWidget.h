///////////////////////////////////////////////////
//
//	Hamish Carr
//	October, 2023
//
//	------------------------
//	AnimationCycleWidget.h
//	------------------------
//	
//	The main widget that shows the geometry
//	
///////////////////////////////////////////////////

#ifndef _GEOMETRIC_WIDGET_H
#define _GEOMETRIC_WIDGET_H

#include <QtGlobal>

// this is necessary to allow compilation in both Qt 5 and Qt 6
#if (QT_VERSION < 0x060000)
#include <QGLWidget>
#define _GEOMETRIC_WIDGET_PARENT_CLASS QGLWidget
#define _GL_WIDGET_UPDATE_CALL updateGL
#else
#include <QtOpenGLWidgets/QOpenGLWidget>
#define _GEOMETRIC_WIDGET_PARENT_CLASS QOpenGLWidget
#define _GL_WIDGET_UPDATE_CALL update
#endif
#include <QTimer>
#include <QMouseEvent>

#include "SceneModel.h"

class AnimationCycleWidget : public _GEOMETRIC_WIDGET_PARENT_CLASS										
	{ // class AnimationCycleWidget
	Q_OBJECT
	public:	
	// we have a single model encapsulating the scene
	SceneModel *theScene;

	// a timer for animation
	QTimer *animationTimer;

	// constructor
	AnimationCycleWidget(QWidget *parent, SceneModel *TheScene);
	
	// destructor
	~AnimationCycleWidget();
			
	protected:
	// called when OpenGL context is set up
	void initializeGL() override;
	// called every time the widget is resized
	void resizeGL(int w, int h) override;
	// called every time the widget needs painting
	void paintGL() override;

	// called when a key is pressed
	void keyPressEvent(QKeyEvent *event) override;

	public slots:
	// slot that gets called when it's time for the next frame
	void nextFrame();
	}; // class AnimationCycleWidget

#endif
