
#include "Renderer.h"

#include <qapplication.h>

#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QLinearGradient>

// Include the OculusVR SDK
#include "OVR.h"
#include "OVR_Kernel.h"
#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"
#include <Kernel/OVR_Math.h>
#include <Kernel/OVR_Alg.h>
#include "Kernel/OVR_KeyCodes.h"

///////////////////////////////////////////////////
// Platform-specific defines go here
///////////////////////////////////////////////////
#if defined(_WIN64) || defined(_WIN32)
#elif __APPLE__
#include "Mac.h"
#elif __linux
#endif
///////////////////////////////////////////////////


Renderer::Renderer(int W, int H, QGLFormat format, QMainWindow *parent) :

	QGLWidget(format, parent), m_parentWindow(parent), m_W(W), m_H(H),
	HaveVisionTracking(false),
	HavePositionTracker(false),
	HaveHMDConnected(false),
	HSWDisplayCurrentlyDisplayed(true),
	StartTrackingCaps(0),
	m_paused(false),
	m_drawGL(false),
	m_shaderManager(NULL)

{
	setMouseTracking(true);
	grabMouse();
	QApplication::setOverrideCursor(Qt::BlankCursor);

	//////////////////////////////////////////////////////
	/// Oculus init
	//////////////////////////////////////////////////////

	ovr_Initialize();
	Hmd = ovrHmd_Create(0);
	if (!Hmd)
	{
		std::cerr << "No HMD detected, creating 'fake' one." << std::endl;
		Hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
		if (!Hmd)
		{
			std::cerr << "HMD-creation failed." << std::endl;
			exit(1);
		}
	}
	std::cout << "Hmd->Resolution.w: " << Hmd->Resolution.w << std::endl;
	std::cout << "Hmd->Resolution.h: " << Hmd->Resolution.h << std::endl;
	m_W = Hmd->Resolution.w;
	m_H = Hmd->Resolution.h;
	resize(m_W, m_H);

	// Hmd caps.
	bool VsyncEnabled = true;
	unsigned hmdCaps = (VsyncEnabled ? 0 : ovrHmdCap_NoVSync);
	{
		bool IsLowPersistence = true;
		if (IsLowPersistence)
			hmdCaps |= ovrHmdCap_LowPersistence;

		// ovrHmdCap_DynamicPrediction - enables internal latency feedback
		bool DynamicPrediction = false;
		if (DynamicPrediction)
			hmdCaps |= ovrHmdCap_DynamicPrediction;

		bool MirrorToWindow = false;
		if (!MirrorToWindow)
			hmdCaps |= ovrHmdCap_NoMirrorToWindow;
	}
	ovrHmd_SetEnabledCaps(Hmd, hmdCaps);

	tStart = 0.0;
	format.setDepth(false);
	setAutoBufferSwap(false);

	// Odd hack required for Mac fullscreen. Works though.
	Mac::fullscreen(this);

	tStart = ovr_GetTimeInSeconds();
	m_lastMousePos = QPoint(width()/2,height()/2);
}

Renderer::~Renderer()
{
	if (Hmd) ovrHmd_Destroy(Hmd);
	ovr_Shutdown();
}

void Renderer::paintGL()
{
	update(); // Force repaint as soon as possible.
}

void Renderer::render()
{
	ovrHSWDisplayState hswDisplayState;
	ovrHmd_GetHSWDisplayState(Hmd, &hswDisplayState);
	if (hswDisplayState.Displayed)
	{
		ovrHmd_DismissHSWDisplay(Hmd);
	}

	static double LastUpdate;
	float  dt;
	double curtime;
	if (m_paused)
	{
		curtime = LastUpdate;
		dt = 0.0666f;
	}
	else
	{
		curtime = ovr_GetTimeInSeconds() - tStart;
		dt = OVR::Alg::Min<float>(float(curtime - LastUpdate), 0.1f);
		LastUpdate = curtime;
	}

	ovrFrameTiming HmdFrameTiming = ovrHmd_BeginFrame(Hmd, 0);

	// Query the HMD for the current tracking state.
	ovrTrackingState ts = ovrHmd_GetTrackingState(Hmd, HmdFrameTiming.ScanoutMidpointSeconds);

	// Report vision tracking
	bool hadVisionTracking = HaveVisionTracking;
	HaveVisionTracking = (ts.StatusFlags & ovrStatus_PositionTracked) != 0;
	if (HaveVisionTracking && !hadVisionTracking)
		std::cout << "Vision Tracking Acquired .." << std::endl;
	if (!HaveVisionTracking && hadVisionTracking)
		std::cout << "Lost Vision Tracking .." << std::endl;

	// Report position tracker
	bool hadPositionTracker = HavePositionTracker;
	HavePositionTracker = (ts.StatusFlags & ovrStatus_PositionConnected) != 0;
	if (HavePositionTracker && !hadPositionTracker)
		std::cout <<  "Position Tracker Connected" << std::endl;
	if (!HavePositionTracker && hadPositionTracker)
		std::cout << "Position Tracker Disconnected" << std::endl;

	// Report position tracker
	bool hadHMDConnected = HaveHMDConnected;
	HaveHMDConnected = (ts.StatusFlags & ovrStatus_HmdConnected) != 0;
	if (HaveHMDConnected && !hadHMDConnected)
		std::cout << "HMD Connected" << std::endl;
	if (!HaveHMDConnected && hadHMDConnected)
		std::cout << "HMD Disconnected" << std::endl;

	// Update pose based on frame!
	m_player.HeadPose = ts.HeadPose.ThePose;

	// Movement/rotation with the gamepad.
	m_player.BodyYaw -= m_player.GamepadRotate.x * dt;
	m_player.HandleMovement(dt, false);

	/////////////////////////////////////////////////////////////////////////////
	/// Render current scene to eye texture
	/////////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glActiveTexture(GL_TEXTURE0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexId, 0);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "render: FBO incomplete..." << std::endl;
	}

	ovrTrackingState hmdState;
	ovrVector3f hmdToEyeViewOffset[2] = { EyeRenderDesc[0].HmdToEyeViewOffset,
										  EyeRenderDesc[1].HmdToEyeViewOffset };

	const float SCALE_FACTOR = 1.f;
	for (int e=0; e<2; e++)
	{
		hmdToEyeViewOffset[e].x *= SCALE_FACTOR;
		hmdToEyeViewOffset[e].y *= SCALE_FACTOR;
		hmdToEyeViewOffset[e].z *= SCALE_FACTOR;
	}

	ovrHmd_GetEyePoses(Hmd, 0, hmdToEyeViewOffset, EyeRenderPose, &hmdState);
	for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
	{
		raytrace(eyeIndex, curtime);
		if (m_drawGL)
		{
			drawGL(eyeIndex, curtime);
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	/// Render eye texture to screen, with appropriate distortion, via OVR SDK
	/////////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ovrTexture EyeTexture[2];
	EyeTexture[0] = eyeGLTextures[0].Texture;
	EyeTexture[1] = eyeGLTextures[1].Texture;
	ovrHmd_EndFrame(Hmd, EyeRenderPose, EyeTexture);

	glFlush();
	glFinish();
}

/////////////////////////////////////////////////////////////////////////////////
/// Implements ShaderToy-style raytracing into the viewport for the given eye
/////////////////////////////////////////////////////////////////////////////////
void Renderer::raytrace(int eyeIndex, double globalTime)
{
	// Render a viewport-aligned quad using a raytracing shader:
	ovrEyeType eye = Hmd->EyeRenderOrder[eyeIndex];

	OVR::Recti renderViewport = eyeGLTextures[eye].Texture.Header.RenderViewport;
	glViewport(renderViewport.x, renderViewport.y, renderViewport.w, renderViewport.h);

	Player::CameraBasis cameraBasis;
	if (eyeIndex==0)
	{
		m_player.calculateCameraBasisFromPose(EyeRenderPose[1], EyeRenderDesc[1], cameraBasis);
	}
	else
	{
		m_player.calculateCameraBasisFromPose(EyeRenderPose[0], EyeRenderDesc[0], cameraBasis);
	}

	GLuint shader = m_shaderManager->getProgram();
	glUseProgram(shader);

	glUniform1f(glGetUniformLocation(shader, "iGlobalTime"), (float)globalTime);
	glUniform1i(glGetUniformLocation(shader, "iPaused"), (int)m_paused);

	float UpTan    = cameraBasis.fov.UpTan;
	float DownTan  = cameraBasis.fov.DownTan;
	float LeftTan  = cameraBasis.fov.LeftTan;
	float RightTan = cameraBasis.fov.RightTan;
	glUniform1f(glGetUniformLocation(shader, "UpTan"), (float)UpTan);
	glUniform1f(glGetUniformLocation(shader, "DownTan"), (float)DownTan);
	glUniform1f(glGetUniformLocation(shader, "LeftTan"), (float)LeftTan);
	glUniform1f(glGetUniformLocation(shader, "RightTan"), (float)RightTan);

	OVR::Vector3f X = cameraBasis.x;
	OVR::Vector3f Y = cameraBasis.y;
	OVR::Vector3f Z = cameraBasis.z;
	glUniform3f(glGetUniformLocation(shader, "camBasisX"), X.x, X.y, X.z);
	glUniform3f(glGetUniformLocation(shader, "camBasisY"), Y.x, Y.y, Y.z);
	glUniform3f(glGetUniformLocation(shader, "camBasisZ"), Z.x, Z.y, Z.z);

	OVR::Vector3f E = cameraBasis.pos;
	glUniform3f(glGetUniformLocation(shader, "camPos"), E.x, E.y, E.z);

	float znear = cameraBasis.znear;
	float zfar = cameraBasis.zfar;
	glUniform1f(glGetUniformLocation(shader, "znear"), (float)znear);
	glUniform1f(glGetUniformLocation(shader, "zfar"), (float)zfar);

	glUniform1f(glGetUniformLocation(shader, "viewportW"), (float)renderViewport.w);
	glUniform1f(glGetUniformLocation(shader, "viewportH"), (float)renderViewport.h);
	glUniform1f(glGetUniformLocation(shader, "viewportX"), (float)renderViewport.x);
	glUniform1f(glGetUniformLocation(shader, "viewportY"), (float)renderViewport.y);
	glUniform2f(glGetUniformLocation(shader, "iResolution"), renderViewport.w, renderViewport.h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBegin (GL_QUADS);
	glVertex2f(-1.f, -1.f);
	glVertex2f( 1.f, -1.f);
	glVertex2f( 1.f,  1.f);
	glVertex2f(-1.f,  1.f);
	glEnd ();

	glUseProgram(0);
}

/////////////////////////////////////////////////////////////////////////////////
/// Normal GL rendering per eye goes here
/// (Currently just overlaid on top of the raytracing)
/////////////////////////////////////////////////////////////////////////////////

void drawGrid(OVR::Vector3f origin, float X, float Y, int numDivisions)
{
	const float dX = X/float(numDivisions);
	const float dY = Y/float(numDivisions);
	const float hX = 0.5f*X;
	const float hY = 0.5f*Y;
	glBegin(GL_LINES);
	for (int i=0; i<=numDivisions; ++i)
	{
		float x = -0.5f*X + dX*float(i);
		float y = -0.5f*Y + dY*float(i);
		glVertex3f(origin.x-hX, origin.z, origin.y+y);
		glVertex3f(origin.x+hX, origin.z, origin.y+y);
		glVertex3f(origin.x+x, origin.z, origin.y-hY);
		glVertex3f(origin.x+x, origin.z, origin.y+hY);
	}
	glEnd();
}

void Renderer::drawGL(int eyeIndex, double globalTime)
{
	ovrEyeType eye = Hmd->EyeRenderOrder[eyeIndex];

	OVR::Matrix4f view = m_player.CalculateViewFromPose(EyeRenderPose[eye]);
	const OVR::Matrix4f viewProj = Projection[eye] * view;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glLoadMatrixf(&viewProj.Transposed().M[0][0]);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	drawGrid(OVR::Vector3f(0), 100.f, 100.f, 256);
}

/*************************************************************************/
/* QGLWidget calls                                                       */
/*************************************************************************/

void Renderer::mousePressEvent(QMouseEvent *event)
{

}

void Renderer::wheelEvent(QWheelEvent *event)
{

}

void Renderer::mouseMoveEvent(QMouseEvent *event)
{
	float dx = float(event->x() - m_lastMousePos.x())/width();
	m_player.BodyYaw -= Sensitivity * M_PI * (dx);

	QPoint center = mapToGlobal(QPoint(width()/2,height()/2));
	QCursor::setPos(center);
	m_lastMousePos = QPoint(width()/2,height()/2);
}

void Renderer::keyPressEvent(QKeyEvent* event)
{
	switch(event->key())
	{
		case Qt::Key_Escape:
			close();
			exit(1);
			break;

		case Qt::Key_Space:
			m_shaderManager->loadNextShader();
			break;

		case Qt::Key_W: m_player.HandleMoveKey(OVR::Key_W, true); break;
		case Qt::Key_S: m_player.HandleMoveKey(OVR::Key_S, true); break;
		case Qt::Key_A: m_player.HandleMoveKey(OVR::Key_A, true); break;
		case Qt::Key_D: m_player.HandleMoveKey(OVR::Key_D, true); break;
		case Qt::Key_Up: m_player.HandleMoveKey(OVR::Key_Up, true); break;
		case Qt::Key_Down: m_player.HandleMoveKey(OVR::Key_Down, true); break;
		case Qt::Key_Left: m_player.HandleMoveKey(OVR::Key_Left, true); break;
		case Qt::Key_Right: m_player.HandleMoveKey(OVR::Key_Right, true); break;

		case Qt::Key_P: m_paused = !m_paused; break;
		case Qt::Key_G: m_drawGL = !m_drawGL; break;

		default:
			event->ignore();
			break;
	}
}


void Renderer::keyReleaseEvent(QKeyEvent *event)
{
	switch(event->key())
	{
		case Qt::Key_W: m_player.HandleMoveKey(OVR::Key_W, false); break;
		case Qt::Key_S: m_player.HandleMoveKey(OVR::Key_S, false); break;
		case Qt::Key_A: m_player.HandleMoveKey(OVR::Key_A, false); break;
		case Qt::Key_D: m_player.HandleMoveKey(OVR::Key_D, false); break;
		case Qt::Key_Up: m_player.HandleMoveKey(OVR::Key_Up, false); break;
		case Qt::Key_Down: m_player.HandleMoveKey(OVR::Key_Down, false); break;
		case Qt::Key_Left: m_player.HandleMoveKey(OVR::Key_Left, false); break;
		case Qt::Key_Right: m_player.HandleMoveKey(OVR::Key_Right, false); break;

		default:
			event->ignore();
			break;
	}
}

void Renderer::initializeGL()
{
	static bool done = false;
	if (!done)
	{
		std::cout << "initializeGL.." << std::endl;

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_COLOR_MATERIAL);
		glDisable(GL_BLEND);
		glEnable(GL_POLYGON_SMOOTH);
		glEnable(GL_MULTISAMPLE);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_MULTISAMPLE_ARB);

		m_shaderManager = new ShaderManager();

		// Configure Oculus rendering
		{
			ovrRenderAPIConfig config;
			config.Header.API = ovrRenderAPI_OpenGL;
			config.Header.RTSize = OVR::Sizei(Hmd->Resolution.w, Hmd->Resolution.h);
			config.Header.Multisample = 1;

			std::cerr << "ovrHmd_ConfigureRendering" << std::endl;
			std::cerr << "width()" << Hmd->Resolution.w << std::endl;
			std::cerr << "height()" << Hmd->Resolution.h << std::endl;

			unsigned distortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette;
			bool SupportsSrgb = true;
			bool PixelLuminanceOverdrive = true;
			bool TimewarpEnabled = true;
			bool TimewarpNoJitEnabled = false;
			if (SupportsSrgb)           distortionCaps |= ovrDistortionCap_SRGB;
			if(PixelLuminanceOverdrive) distortionCaps |= ovrDistortionCap_Overdrive;
			if (TimewarpEnabled)        distortionCaps |= ovrDistortionCap_TimeWarp;
			if(TimewarpNoJitEnabled)    distortionCaps |= ovrDistortionCap_ProfileNoTimewarpSpinWaits;

			if (!ovrHmd_ConfigureRendering(Hmd, &config, distortionCaps, Hmd->DefaultEyeFov, EyeRenderDesc))
			{
				std::cerr << "ovrHmd_ConfigureRendering" << std::endl;
				exit(1);
			}

			unsigned sensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection;
			bool PositionTrackingEnabled = false;
			if (PositionTrackingEnabled)
				sensorCaps |= ovrTrackingCap_Position;
			if (StartTrackingCaps != sensorCaps)
			{
				ovrHmd_ConfigureTracking(Hmd, sensorCaps, 0);
				StartTrackingCaps = sensorCaps;
			}

			Projection[0] = ovrMatrix4f_Projection(EyeRenderDesc[0].Fov, .1f, 1000.0f, true);
			Projection[1] = ovrMatrix4f_Projection(EyeRenderDesc[1].Fov, .1f, 1000.0f, true);
		}

		// Initialize eye rendering information for ovrHmd_Configure.
		// The viewport sizes are re-computed in case RenderTargetSize changed due to HW limitations.
		ovrFovPort eyeFov[2];
		eyeFov[0] = Hmd->DefaultEyeFov[0];
		eyeFov[1] = Hmd->DefaultEyeFov[1];

		OVR::Sizei recommendedTexSize[] = {
			  ovrHmd_GetFovTextureSize(Hmd, ovrEye_Left, eyeFov[0], 1),
			  ovrHmd_GetFovTextureSize(Hmd, ovrEye_Right, eyeFov[1], 1)
		  };

		std::cout << "recommendedTexSize[0]: " << recommendedTexSize[0].w << ", " << recommendedTexSize[0].h << std::endl;
		std::cout << "recommendedTexSize[1]: " << recommendedTexSize[1].w << ", " << recommendedTexSize[1].h << std::endl;

		OVR::Sizei  rtSize(recommendedTexSize[0].w + recommendedTexSize[1].w,
							OVR::Alg::Max(recommendedTexSize[0].h, recommendedTexSize[1].h));

		EyeRenderSize[0] = OVR::Sizei::Min(OVR::Sizei(rtSize.w/2, rtSize.h), recommendedTexSize[0]);
		EyeRenderSize[1] = OVR::Sizei::Min(OVR::Sizei(rtSize.w/2, rtSize.h), recommendedTexSize[1]);

		// Store texture pointers that will be passed for rendering.
		// Same texture is used, but with different viewports.

		{
			glGenTextures(1, &m_colorTexId);
			glBindTexture(GL_TEXTURE_2D, m_colorTexId);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rtSize.w, rtSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		}
		{
			glGenTextures(1, &m_depthTexId);
			glBindTexture(GL_TEXTURE_2D, m_depthTexId);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, rtSize.w, rtSize.h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		}

		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexId, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexId, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "initializeGL: FBO incomplete..." << std::endl;
		}

		// Set up the two ovrGLTexture structs for ovrHmd_EndFrame to operate on:
		eyeGLTextures[0].OGL.Header.RenderViewport = OVR::Recti(OVR::Vector2i(0), EyeRenderSize[0]);
		eyeGLTextures[1].OGL.Header.RenderViewport = OVR::Recti(OVR::Vector2i((rtSize.w+1)/2, 0), EyeRenderSize[1]);
		for (int i=0; i<2; ++i)
		{
			eyeGLTextures[i].OGL.Header.API = ovrRenderAPI_OpenGL;
			eyeGLTextures[i].OGL.Header.TextureSize = rtSize;
			eyeGLTextures[i].OGL.TexId = m_colorTexId;
		}

		QTimer *t = new QTimer(this);
		connect(t, &QTimer::timeout, this, &QGLWidget::updateGL);
		t->start();

		GLenum gl_err = glGetError();
		if (gl_err != GL_NO_ERROR)
		{
			printf("GL error: %d/n", gl_err);
			exit(1);
		}

		std::cout << "Renderer::initializeGL done" << std::endl;

		done = true;
	}
}

void Renderer::resizeGL(int width, int height)
{

}



