
#pragma once

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

class ShaderManager
{
public:

	ShaderManager();

	GLuint getProgram() { return g_program; }

private:

	GLuint g_program;

	static GLuint shaderCompileFromFile(GLenum type, const char *filePath);
	static void shaderAttachFromFile(GLuint program, GLenum type, const char *filePath);


};

