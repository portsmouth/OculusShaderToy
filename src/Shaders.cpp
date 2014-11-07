
#include "Shaders.h"

#include <QDirIterator>

/*
* Returns a string containing the text in
* a vertex/fragment shader source file.
*/
static char* shaderLoadSource(const char *filePath)
{
	const size_t blockSize = 512;
	FILE *fp;
	char buf[blockSize];
	char *source = NULL;
	size_t tmp, sourceLength = 0;
	/* open file */
	fp = fopen(filePath, "r");
	if(!fp) {
		fprintf(stderr, "shaderLoadSource(): Unable to open %s for reading\n", filePath);
		return NULL;
	}
	/* read the entire file into a string */
	while((tmp = fread(buf, 1, blockSize, fp)) > 0)
	{
		char *newSource = (char*)malloc(sourceLength + tmp + 1);
		if(!newSource)
		{
			fprintf(stderr, "shaderLoadSource(): malloc failed\n");
			if(source)
				free(source);
			return NULL;
		}
		if(source) {
			memcpy(newSource, source, sourceLength);
			free(source);
		}
		memcpy(newSource + sourceLength, buf, tmp);
		source = newSource;
		sourceLength += tmp;
	}
	/* close the file and null terminate the string */
	fclose(fp);
	if(source)
		source[sourceLength] = '\0';
	return source;
}


GLuint ShaderManager::shaderCompileFromFile(GLenum type, const char *filePath)
{
	GLchar *source;
	GLuint shader;
	GLint length;

	/* get shader source */
	source = shaderLoadSource(filePath);
	if(!source)
		return 0;

	/* create shader object, set the source, and compile */
	shader = glCreateShader(type);

	glShaderSource(shader, 1, (const GLchar **)&source, NULL);
	glCompileShader(shader);
	free(source);

	/* make sure the compilation was successful */
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE)
	{
		/* get the shader info log */
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		std::cout << "maxLength: " << maxLength << std::endl;

		GLchar* log_string = new char[maxLength+1];
		glGetShaderInfoLog(shader, maxLength, &maxLength, log_string);

		/* print an error message and the info log */
		fprintf(stderr, "shaderCompileFromFile(): Unable to compile %s: \n%s", filePath, log_string);

		 delete log_string;

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}


/*
* Compiles and attaches a shader of the
* given type to the given program object.
*/
void ShaderManager::shaderAttachFromFile(GLuint program, GLenum type, const char *filePath)
{
	std::cout << "Compile and attach shader " << filePath << std::endl;

	/* compile the shader */
	GLuint shader = ShaderManager::shaderCompileFromFile(type, filePath);
	if(shader != 0)
	{
		/* attach the shader to the program */
		glAttachShader(program, shader);

		/* delete the shader - it won't actually be
		* destroyed until the program that it's attached
		* to has been destroyed */
		glDeleteShader(shader);

		std::cout << "Successfully attached shader" << std::endl;
	}
}


void findFragmentShaders(std::string baseDir, std::vector<std::string>& pathsOut)
{
	QStringList nameFilter("*.fs");
	QDir dir(baseDir.c_str());
	QStringList files = dir.entryList(nameFilter);
	for(int i=0;i<files.size();i++)
	{
		std::string path = dir.absoluteFilePath(files[i]).toUtf8().constData();
		pathsOut.push_back(path);
	}
}



void ShaderManager::loadNextShader()
{
	m_currentShader++;
	if (m_currentShader>=m_fragmentShaders.size())
		m_currentShader=0;

	hotLoad(m_currentShader);
}


void ShaderManager::hotLoad(unsigned int n)
{
	/* create program object and attach shaders */
	g_program = glCreateProgram();
	shaderAttachFromFile(g_program, GL_VERTEX_SHADER, "/Users/jamports/projects/OculusExperiments/shaders/shader.vs");

	std::string fragmentShaderPath = m_fragmentShaders[n];
	shaderAttachFromFile(g_program, GL_FRAGMENT_SHADER, fragmentShaderPath.c_str());

	/* link the program and make sure that there were no errors */
	GLint result;
	glLinkProgram(g_program);
	glGetProgramiv(g_program, GL_LINK_STATUS, &result);
	if(result == GL_FALSE)
	{
		GLint length;
		char *log;

		/* get the program info log */
		glGetProgramiv(g_program, GL_INFO_LOG_LENGTH, &length);
		log = (char*)malloc(length);
		glGetProgramInfoLog(g_program, length, &result, log);

		/* print an error message and the info log */
		fprintf(stderr, "ShaderManager(): Program linking failed: %s\n", log);
		free(log);

		/* delete the program */
		glDeleteProgram(g_program);
		g_program = 0;
	}

	std::cout << "Successfully linked shader program." << std::endl;
}



ShaderManager::ShaderManager() : m_currentShader(0)
{
	std::string shaderDir = "/Users/jamports/projects/OculusExperiments/shaders/";
	findFragmentShaders(shaderDir, m_fragmentShaders);
	for (int n=0; n<m_fragmentShaders.size(); ++n)
	{
		std::cout << "Found shader: " << m_fragmentShaders[n] << std::endl;
	}

	hotLoad(m_currentShader);
}
