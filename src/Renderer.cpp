
#include "Renderer.h"

#include <qapplication.h>

#include <ImathMatrix.h>
#include <ImathBox.h>
#include <ImathVec.h>

#if defined(_WIN64) || defined(_WIN32)
#elif __APPLE__
#elif __linux
#endif

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

Renderer::Renderer(int W, int H, QGLFormat format, QMainWindow *parent) :

	QGLWidget(format, parent), m_parentWindow(parent), m_W(W), m_H(H),
	HaveVisionTracking(false),
	HavePositionTracker(false),
	HaveHMDConnected(false),
	StartTrackingCaps(0),
	m_fullscreened(false),
	HSWDisplayCurrentlyDisplayed(true),

	m_shaderManager(NULL)

{
	setMouseTracking(true);
	grabMouse();
	setFocusPolicy(Qt::StrongFocus);

	QApplication::setOverrideCursor(Qt::BlankCursor);


	//////////////////////////////////////////////////////
	/// Oculus init
	//////////////////////////////////////////////////////

	std::cout << "before" << std::endl;
	ovr_Initialize();
	std::cout << "after" << std::endl;

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
	m_lastMousePos = QPoint(width()/2,height()/2);

	// Hmd caps.
	bool VsyncEnabled = true;
	unsigned hmdCaps = (VsyncEnabled ? 0 : ovrHmdCap_NoVSync);

	bool IsLowPersistence = true;
	if (IsLowPersistence)
		hmdCaps |= ovrHmdCap_LowPersistence;

	// ovrHmdCap_DynamicPrediction - enables internal latency feedback
	bool DynamicPrediction = false;
	if (DynamicPrediction)
		hmdCaps |= ovrHmdCap_DynamicPrediction;

	// ovrHmdCap_DisplayOff - turns off the display
	//bool DisplaySleep = false;
	//if (DisplaySleep)
	//	hmdCaps |= ovrHmdCap_DisplayOff;

	bool MirrorToWindow = true;
	if (!MirrorToWindow)
		hmdCaps |= ovrHmdCap_NoMirrorToWindow;

	ovrHmd_SetEnabledCaps(Hmd, hmdCaps);

	tStart = 0.0;

	format.setDepth(false);
	setAutoBufferSwap(false);

}

Renderer::~Renderer()
{
	makeCurrent();
	//delete offFB;

	if (Hmd) ovrHmd_Destroy(Hmd);
	ovr_Shutdown();
}


void drawGrid(Imath::V3f origin, float X, float Y, int numDivisions)
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


void Renderer::paintGL()
{
	render();
	makeCurrent();
	swapBuffers();
	update(); // Force repaint as soon as possible.
}


OVR::Matrix4f Renderer::CalculateViewFromPose(const OVR::Posef& pose)
{
	OVR::Posef worldPose = m_player.VirtualWorldTransformfromRealPose(pose);

	// Rotate and position View Camera
	OVR::Vector3f up      = worldPose.Rotation.Rotate(UpVector);
	OVR::Vector3f forward = worldPose.Rotation.Rotate(ForwardVector);

	// Transform the position of the center eye in the real world (i.e. sitting in your chair)
	// into the frame of the player's virtual body.
	OVR::Vector3f viewPos = worldPose.Translation;
	OVR::Matrix4f view = OVR::Matrix4f::LookAtRH(viewPos, viewPos + forward, up);

	return view;
}


void Renderer::calculateCameraBasisFromPose(const OVR::Posef& pose, const ovrEyeRenderDesc& eyeDesc,
											CameraBasis& cameraBasis)
{
	OVR::Posef worldPose = m_player.VirtualWorldTransformfromRealPose(pose);

	OVR::Vector3f forward = worldPose.Rotation.Rotate(ForwardVector);
	OVR::Vector3f viewPos = worldPose.Translation;
	cameraBasis.pos = viewPos;

	OVR::Vector3f up      = worldPose.Rotation.Rotate(UpVector);
	OVR::Vector3f eye = viewPos;
	OVR::Vector3f at = viewPos + forward;

	cameraBasis.z = (eye - at).Normalized();  // Forward
	cameraBasis.x = up.Cross(cameraBasis.z).Normalized(); // Right
	cameraBasis.y = cameraBasis.z.Cross(cameraBasis.x);

	cameraBasis.fov = eyeDesc.Fov;
	cameraBasis.znear = .1f;
	cameraBasis.zfar = 1000.0f;

}


void Renderer::render()
{
	static int frameNum=0;
	//std::cout << "Renderer::render, frame " << frameNum++ << std::endl;

	this->makeCurrent();

	ovrHSWDisplayState hswDisplayState;
	ovrHmd_GetHSWDisplayState(Hmd, &hswDisplayState);
	if (hswDisplayState.Displayed)
	{
		ovrHmd_DismissHSWDisplay(Hmd);
	}

	float defaultEyeHeight = 1.7;
	float userEyeHeight           = ovrHmd_GetFloat(Hmd, OVR_KEY_EYE_HEIGHT, defaultEyeHeight);
	float centerPupilDepthMeters  = ovrHmd_GetFloat(Hmd, "CenterPupilDepth", 0.0f);

	static double LastUpdate;
	double curtime = ovr_GetTimeInSeconds();

	// If running slower than 10fps, clamp. Helps when debugging, because then dt can be minutes!
	float  dt = OVR::Alg::Min<float>(float(curtime - LastUpdate), 0.1f);
	LastUpdate = curtime;

	ovrFrameTiming HmdFrameTiming = ovrHmd_BeginFrame(Hmd, 0);

	// Query the HMD for the current tracking state.
	ovrTrackingState ts = ovrHmd_GetTrackingState(Hmd, HmdFrameTiming.ScanoutMidpointSeconds);
	unsigned int HmdStatus = ts.StatusFlags;

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

	//////////////////////////////////////////////////
	/// Render current scene
	//////////////////////////////////////////////////

	glUseProgram(0); // Fixed function pipeline
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	glClearColor(0.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);

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
	ovrHmd_GetEyePoses(Hmd, 0, hmdToEyeViewOffset, EyeRenderPose, &hmdState);

	//std::cout << "\n-------------------------------" << std::endl;

	for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
	{
		ovrEyeType eye = Hmd->EyeRenderOrder[eyeIndex];
		//std::cout << "\neye: " << eye << std::endl;

		//OVR::Matrix4f view = CalculateViewFromPose(EyeRenderPose[eye]);
		OVR::Recti renderViewport = eyeGLTextures[eye].Texture.Header.RenderViewport;
		glViewport(renderViewport.x, renderViewport.y, renderViewport.w, renderViewport.h);

		/*
		std::cout << "renderViewport.x: " << renderViewport.x << std::endl;
		std::cout << "renderViewport.y: " << renderViewport.y << std::endl;
		std::cout << "renderViewport.w: " << renderViewport.w << std::endl;
		std::cout << "renderViewport.h: " << renderViewport.h << std::endl;
		*/

		//const OVR::Matrix4f viewProj = Projection[eye] * view;

		CameraBasis cameraBasis;
		calculateCameraBasisFromPose(EyeRenderPose[eye], EyeRenderDesc[eye], cameraBasis);

		/*
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glLoadMatrixf(&viewProj.Transposed().M[0][0]);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		*/

		//eyeIndex==0 ? glColor3f(1.f, 0.f, 0.f) : glColor3f(0.f, 1.f, 0.f);

		drawScene(eye, dt, curtime, renderViewport, cameraBasis);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ovrTexture EyeTexture[2];
	EyeTexture[0] = eyeGLTextures[0].Texture;
	EyeTexture[1] = eyeGLTextures[1].Texture;
	ovrHmd_EndFrame(Hmd, EyeRenderPose, EyeTexture);

	m_repaintFlag = false;

	glFlush();
	glFinish();
}


void Renderer::drawScene(ovrEyeType eye, const double &dt, double globalTime,
						 OVR::Recti& renderViewport,
						 CameraBasis& cameraBasis)
{
	// Render a viewport-aligned quad using a raytracing shader:
	GLuint shader = m_shaderManager->getProgram();
	glUseProgram(shader);

	glUniform1f(glGetUniformLocation(shader, "iGlobalTime"), (float)globalTime);

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

	/*
	std::cout << "camX: " << X.x << ", " << X.y << ", " << X.z << std::endl;
	std::cout << "camY: " << Y.x << ", " << Y.y << ", " << Y.z << std::endl;
	std::cout << "camZ: " << Z.x << ", " << Z.y << ", " << Z.z << std::endl;
	std::cout << "camPos: " << E.x << ", " << E.y << ", " << E.z << std::endl;

	std::cout << "UpTan: " << UpTan << std::endl;
	std::cout << "DownTan: " << DownTan << std::endl;
	std::cout << "LeftTan: " << LeftTan << std::endl;
	std::cout << "RightTan: " << RightTan << std::endl;

	std::cout << "znear: " << znear << std::endl;
	std::cout << "zfar: " << zfar << std::endl;
	*/

	glBegin (GL_QUADS);
	glVertex2f(-1.f, -1.f);
	glVertex2f( 1.f, -1.f);
	glVertex2f( 1.f,  1.f);
	glVertex2f(-1.f,  1.f);
	glEnd ();

	glUseProgram(0);

	//drawGrid(Imath::V3f(0), 100.f, 100.f, 256);
}



/*************************************************************************/
/* QGLWidget calls                                                       */
/*************************************************************************/

void Renderer::mousePressEvent(QMouseEvent *event)
{
	m_lastMousePos = event->pos();
}

void Renderer::wheelEvent(QWheelEvent *event)
{
	/*
	// mouse wheel = zoom in and out
	m_camera->goForward(m_camera->m_stepLength * event->delta());
	m_repaintFlag = true; // Indicates the progressive update should be started fresh
	paintGL();
	*/
}

void Renderer::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - m_lastMousePos.x();
	int dy = event->y() - m_lastMousePos.y();

	// Alt+left mouse button = Rotate
	// Alt+middle mouse button = Pan
	//Qt::KeyboardModifiers key_modifiers = qApp->keyboardModifiers();

	//if (key_modifiers.testFlag(Qt::AltModifier))
	{
		//if (event->buttons().testFlag(Qt::LeftButton))
		{
			m_player.BodyYaw -= (Sensitivity * dx) / 360.0f;
		}
	}

	//m_lastMousePos = event->pos();

	/*
	QPoint glob = mapToGlobal(QPoint(width()/2,height()/2));
		QCursor::setPos(glob);
		m_lastMousePos = QPoint(width()/2,height()/2);
		QGLWidget::mouseMoveEvent(event);
	*/
	paintGL();
}


void Renderer::keyPressEvent(QKeyEvent* event)
{
	switch(event->key())
	{
		case Qt::Key_Escape:
			exit(1); // temp hack!
			close();
			break;

		case Qt::Key_F9:
			if (!m_fullscreened)
			{
				m_parentWindow->showFullScreen();
				QGLWidget::showFullScreen();
			}
			else
			{
				m_parentWindow->showNormal();
				QGLWidget::showNormal();
			}
			m_fullscreened = !m_fullscreened;
			break;

		case Qt::Key_W: m_player.HandleMoveKey(OVR::Key_W, true); break;
		case Qt::Key_S: m_player.HandleMoveKey(OVR::Key_S, true); break;
		case Qt::Key_A: m_player.HandleMoveKey(OVR::Key_A, true); break;
		case Qt::Key_D: m_player.HandleMoveKey(OVR::Key_D, true); break;
		case Qt::Key_Up: m_player.HandleMoveKey(OVR::Key_Up, true); break;
		case Qt::Key_Down: m_player.HandleMoveKey(OVR::Key_Down, true); break;
		case Qt::Key_Left: m_player.HandleMoveKey(OVR::Key_Left, true); break;
		case Qt::Key_Right: m_player.HandleMoveKey(OVR::Key_Right, true); break;

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
	std::cout << "initializeGL.." << std::endl;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	//glEnable(GL_MULTISAMPLE);
	//glShadeModel(GL_SMOOTH);

	//glEnable(GL_MULTISAMPLE_ARB);

	GLenum gl_err = glGetError();

	glFinish();

	if (gl_err != GL_NO_ERROR)
	{
		printf("GL error: %d/n", gl_err);
		exit(1);
	}
	printf("GL initialization complete.\n");

	//////////////////////////////////
	/// Setup shader
	//////////////////////////////////

	m_shaderManager = new ShaderManager();

	//glShadeModel(GL_SMOOTH);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	///
	{
		//glViewport(0, 0, m_W, m_H);
		//glMatrixMode(GL_MODELVIEW);
		//glLoadIdentity();

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

	std::cout << "Renderer::initializeGL done" << std::endl;
}




void Renderer::resizeGL(int width, int height)
{
	std::cout << "Renderer::resizeGL (" << width << ", " << height << ")" << std::endl;

}



