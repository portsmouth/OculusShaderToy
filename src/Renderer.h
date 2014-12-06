
#pragma once

#include <QMainWindow>
#include <QtOpenGL/QGLWidget>
#include <QtGui/QMouseEvent>
#include <QGLFramebufferObject>
#include <QTimer>
#include <QtOpenGLExtensions/QOpenGLExtensions>

///////////////////////////////////////////////////
// Platform-specific defines go here
///////////////////////////////////////////////////
#if defined(_WIN64) || defined(_WIN32)
#elif __APPLE__
// Apple only
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenCL/opencl.h>
#include <CGLCurrent.h>
#include <CGLTypes.h>
#elif __linux
#endif
///////////////////////////////////////////////////

// Include the OculusVR SDK
#include "OVR.h"
#include "OVR_Kernel.h"
#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"
#include <Kernel/OVR_Math.h>
#include <Kernel/OVR_Alg.h>
#include "Kernel/OVR_KeyCodes.h"

#include <vector>

#include "Player.h"
#include "Shaders.h"

class Scene;

class Renderer : public QGLWidget
{
	Q_OBJECT // must include this if you use Qt signals/slots

public:

	Renderer(int W, int H, QGLFormat format, QMainWindow *parentWindow = NULL);

	virtual ~Renderer();
	void render();

protected:

	friend class GraphicsView;
	QMainWindow* m_parentWindow;
	int m_W;
	int m_H;

	///////////////////////////////////////////////////////////
	/// Oculus HMD and rendering data
	///////////////////////////////////////////////////////////
	ovrHmd              Hmd;
	ovrEyeRenderDesc    EyeRenderDesc[2];
	OVR::Matrix4f       Projection[2];          // Projection matrix for eye.
	OVR::Matrix4f       OrthoProjection[2];     // Projection for 2D.
	ovrPosef            EyeRenderPose[2];       // Poses we used for rendering.
	ovrGLTexture        eyeGLTextures[2];
	OVR::Sizei          EyeRenderSize[2];
	int m_texwidth;
	int m_texheight;
	GLuint m_fbo;
	GLuint m_colorTexId;
	GLuint m_depthTexId;

	// Current application state
	double tStart;
	QPoint m_lastMousePos;
	bool m_paused;
	bool m_drawGL;
	bool                HaveVisionTracking;
	bool                HavePositionTracker;
	bool                HaveHMDConnected;
	bool                HSWDisplayCurrentlyDisplayed;
	unsigned            StartTrackingCaps;

	// Manages player motion state
	Player m_player;

	// Loads/compiles the raytracing fragment shaders on demand
	ShaderManager* m_shaderManager;

	void drawGL(int eyeIndex, double globalTime);
	void raytrace(int eyeIndex, double globalTime);
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

	// Qt events
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void wheelEvent(QWheelEvent *);

};









