
#pragma once

#include <QMainWindow>

#include <QtOpenGL/QGLWidget>

#include <QtGui/QMouseEvent>
#include <QGLFramebufferObject>
#include <QTimer>

///////////////////////////////////////////////////
// Platform-specific defines go here
///////////////////////////////////////////////////
#if defined(_WIN64) || defined(_WIN32)
#include <GL/glx.h>

#elif __APPLE__
// Apple only
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenCL/opencl.h>

#include <CGLCurrent.h>
#include <CGLTypes.h>

#elif __linux
#include <GL/glx.h>
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

#include <ImathMatrix.h>
#include <ImathVec.h>
#include <ImathBox.h>

#include <vector>

#include "Player.h"
#include "Shaders.h"

class Camera;
class Scene;


class Renderer : public QGLWidget
{
	Q_OBJECT // must include this if you use Qt signals/slots

public:

	Renderer(int W, int H, QGLFormat format, QMainWindow *parentWindow = NULL);

	virtual ~Renderer();

	void render();

protected:

	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

	friend class GraphicsView;
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void wheelEvent(QWheelEvent *);

	bool m_fullscreened;
	QPoint m_lastMousePos;
	Camera* m_camera;
	QMainWindow* m_parentWindow;

	int m_W;
	int m_H;


	// ***** Oculus HMD Variables
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

	double tStart;

	// Current status flags
	bool                HaveVisionTracking;
	bool                HavePositionTracker;
	bool                HaveHMDConnected;
	bool                HSWDisplayCurrentlyDisplayed;

	unsigned            StartTrackingCaps;

	// Class holding the player state
	Player m_player;

	ShaderManager* m_shaderManager;

	OVR::Matrix4f CalculateViewFromPose(const OVR::Posef& pose);

	struct CameraBasis
	{
		OVR::Vector3f pos;
		OVR::Vector3f x, y, z; // direction basis (-z=view, LH)
		ovrFovPort fov;
		float znear, zfar;
	};

	void calculateCameraBasisFromPose(const OVR::Posef& pose,
									  const ovrEyeRenderDesc& eyeDesc,
									  CameraBasis& cameraBasis);

	void drawScene(int eyeIndex, double globalTime);
	void raytrace(int eyeIndex, double globalTime);

};









