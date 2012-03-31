#include "GLProgramFactory.h"
#include "glprogram/ARBBumpProgram.h"
#include "glprogram/ARBDepthFillProgram.h"
#include "glprogram/GLSLDepthFillProgram.h"
#include "glprogram/GLSLBumpProgram.h"

#include "iregistry.h"
#include "imodule.h"
#include "os/file.h"
#include "string/convert.h"
#include "debugging/debugging.h"

#include <fstream>

namespace render
{

// Constructor, populates map with GLProgram instances
GLProgramFactory::GLProgramFactory()
{
    setUsingGLSL(false);
}

GLProgramFactory& GLProgramFactory::instance() 
{
	static GLProgramFactory _instance;
	return _instance;
}

GLProgram* GLProgramFactory::getProgram(const std::string& name)
{
	// Lookup the program, if not found throw an exception
	ProgramMap::iterator i = _map.find(name);
	if (i != _map.end())
		return i->second.get();
	else
		throw std::runtime_error("GLProgramFactory: failed to find program "
								 + name);
}

void GLProgramFactory::setUsingGLSL(bool useGLSL)
{
    if (useGLSL)
    {
        _map["depthFill"] = GLProgramPtr(new GLSLDepthFillProgram());
        _map["bumpMap"] = GLProgramPtr(new GLSLBumpProgram());
    }
    else
    {
        _map["depthFill"] = GLProgramPtr(new ARBDepthFillProgram());
        _map["bumpMap"] = GLProgramPtr(new ARBBumpProgram());
    }
}

// Realise the program factory.
void GLProgramFactory::realise()
{
	// Realise each GLProgram in the map
	for (ProgramMap::iterator i = _map.begin();
		 i != _map.end();
		 ++i)
	{
		i->second->create();
	}
}

// Unrealise the program factory.
void GLProgramFactory::unrealise() 
{
	// Destroy each GLProgram in the map
	for (ProgramMap::iterator i = _map.begin();
		 i != _map.end();
		 ++i)
	{
		i->second->destroy();
	}
}

// Get file as a char buffer
GLProgramFactory::CharBufPtr
GLProgramFactory::getFileAsBuffer(const std::string& filename,
                                  bool nullTerminated)
{
    // Get absolute path from filename
    std::string absFileName = getGLProgramPath(filename);

    // Open the file
	std::size_t size = file_size(absFileName.c_str());
	std::ifstream file(absFileName.c_str());

    // Throw an exception if the file could not be found
	if (!file.is_open())
    {
        throw std::runtime_error(
            "GLProgramFactory::createARBProgram() failed to open file: "
            + absFileName
        );
    }

    // Read the file data into a buffer, adding a NULL terminator if required
    std::size_t bufSize = (nullTerminated ? size + 1 : size);
	CharBufPtr buffer(new std::vector<char>(bufSize, 0));
	file.read(&buffer->front(), size);

    // Close file and return buffer
    file.close();
    return buffer;
}

void GLProgramFactory::assertShaderCompiled(GLuint shader)
{
    // Get compile status
    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

    // Throw exception with log if it failed
    if (compileStatus != GL_TRUE)
    {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        // Get log chars in buffer
        std::vector<char> logBuf(logLength + 1, 0);
        glGetShaderInfoLog(shader, static_cast<GLsizei>(logBuf.size()), NULL, &logBuf.front());

        // Convert to string and throw exception
        std::string logStr = std::string(&logBuf.front());
        throw std::runtime_error(
            "Failed to compile GLSL shader:\n"
            + logStr
        );
    }
}

std::string GLProgramFactory::getProgramInfoLog(GLuint program)
{
    // Get log length
    int logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

    // Get log chars in buffer
    std::vector<char> logBuf(logLength + 1, 0);
    glGetProgramInfoLog(program, static_cast<GLsizei>(logBuf.size()), NULL, &logBuf.front());

    // Convert to string and return
    std::string logStr = std::string(&logBuf.front());
    return logStr;
}

void GLProgramFactory::assertProgramLinked(GLuint program)
{
    // Check the link status
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE)
    {
        throw std::runtime_error(
            "Failed to construct GLSL program:\n"
            + getProgramInfoLog(program)
        );
    }

#ifdef _DEBUG

    // Ask GL to validate the program (this means that it will run)
    glValidateProgram(program);

    // Get the valid status and info log
    GLint validStatus;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &validStatus);

    std::string validLog = getProgramInfoLog(program);

    // Output to console
    std::cout << "[renderer] GLSL program "
              << (validStatus == GL_TRUE ? "IS " : "IS NOT ") << "valid.\n";
    std::cout << "Info:\n" << validLog << std::endl;

#endif
}

GLuint GLProgramFactory::createGLSLProgram(const std::string& vFile,
                                           const std::string& fFile)
{
    // Create the parent program object
    GLuint program = glCreateProgram();

    // Create the shader objects
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    // Load the source files as NULL-terminated strings and pass the text to
    // OpenGL
    CharBufPtr vertexSrc = getFileAsBuffer(vFile, true);
    CharBufPtr fragSrc = getFileAsBuffer(fFile, true);

    const char* csVertex = &vertexSrc->front();
    const char* csFragment = &fragSrc->front();

    glShaderSource(vertexShader, 1, &csVertex, NULL);
    glShaderSource(fragmentShader, 1, &csFragment, NULL);
    GlobalOpenGL().assertNoErrors();

    // Compile the shaders
    glCompileShader(vertexShader);
    assertShaderCompiled(vertexShader);

    glCompileShader(fragmentShader);
    assertShaderCompiled(fragmentShader);

    GlobalOpenGL().assertNoErrors();

    // Attach and link the program object itself
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    GlobalOpenGL().assertNoErrors();

    glLinkProgram(program);

    // Check the link status and throw an exception if it failed
    assertProgramLinked(program);

    // Return the linked program
    return program;
}

GLuint GLProgramFactory::createARBProgram(const std::string& filename,
                                          GLenum type)
{
    // Get the file contents without NULL terminator
    CharBufPtr buffer = getFileAsBuffer(filename, false);

    // Bind the program data into OpenGL
    GlobalOpenGL().assertNoErrors();

    GLuint programID;
    glGenProgramsARB(1, &programID);
    glBindProgramARB(type, programID);

	glProgramStringARB(
        type,
        GL_PROGRAM_FORMAT_ASCII_ARB,
        GLsizei(buffer->size()),
        &buffer->front()
    );

    // Check for GL errors and throw exception if there is a problem
	if (GL_INVALID_OPERATION == glGetError())
    {
		GLint errPos;
		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
		const GLubyte* errString = glGetString(GL_PROGRAM_ERROR_STRING_ARB);

        // Construct user-readable error string
        std::string error("GL program error: ");
        error += filename + "(" + string::to_string(errPos) + "): \n\n";
        error += std::string(reinterpret_cast<const char*>(errString));

        // Throw exception
        //throw std::logic_error(error);
        std::cerr << error << std::endl;
	}

    // Return the new program
    return programID;
}

// Get the path of a GL program file
std::string GLProgramFactory::getGLProgramPath(const std::string& progName)
{
    // Append the requested filename with the "gl/" directory.
    return module::GlobalModuleRegistry()
            .getApplicationContext()
                .getRuntimeDataPath()
                    + "gl/" + progName;
}

} // namespace render
