#include "GLProgramFactory.h"
#include "glprogram/ARBBumpProgram.h"
#include "glprogram/ARBDepthFillProgram.h"

#include "iregistry.h"
#include "os/file.h"
#include "string/string.h"
#include "debugging/debugging.h"

#include <fstream>

namespace render
{

// Constructor, populates map with GLProgram instances
GLProgramFactory::GLProgramFactory()
{
	_map.insert(std::make_pair("depthFill", new ARBDepthFillProgram())); 
	_map.insert(std::make_pair("bumpMap", new ARBBumpProgram()));
}

// Return static GLProgramFactory instance
GLProgramFactory& GLProgramFactory::getInstance() {
	static GLProgramFactory _instance;
	return _instance;
}

// Lookup a named program in the singleton instance
GLProgramPtr GLProgramFactory::getProgram(const std::string& name) {
	
	// Reference to static instance's map
	ProgramMap& map = getInstance()._map;
	
	// Lookup the program, if not found throw an exception
	ProgramMap::iterator i = map.find(name);
	if (i != map.end())
		return i->second;
	else
		throw std::runtime_error("GLProgramFactory: failed to find program "
								 + name);
}

// Realise the program factory.
void GLProgramFactory::realise() {
	
	// Get static map
	ProgramMap& map = getInstance()._map;
	
	// Realise each GLProgram in the map
	for (ProgramMap::iterator i = map.begin();
		 i != map.end();
		 ++i)
	{
		i->second->create();
	}
}

// Unrealise the program factory.
void GLProgramFactory::unrealise() {
	
	// Get static map
	ProgramMap& map = getInstance()._map;
	
	// Destroy each GLProgram in the map
	for (ProgramMap::iterator i = map.begin();
		 i != map.end();
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

#ifdef RADIANT_USE_GLSL

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
        glGetShaderInfoLog(shader, logBuf.size(), NULL, &logBuf.front());

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
    glGetProgramInfoLog(program, logBuf.size(), NULL, &logBuf.front());

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
    GlobalOpenGL_debugAssertNoErrors();

    // Compile the shaders
    glCompileShader(vertexShader);
    assertShaderCompiled(vertexShader);

    glCompileShader(fragmentShader);
    assertShaderCompiled(fragmentShader);

    GlobalOpenGL_debugAssertNoErrors();

    // Attach and link the program object itself
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    GlobalOpenGL_debugAssertNoErrors();

    glLinkProgram(program);

    // Check the link status and throw an exception if it failed
    assertProgramLinked(program);

    // Return the linked program
    return program;
}

#else

GLuint GLProgramFactory::createARBProgram(const std::string& filename,
                                          GLenum type) 
{
    // Get the file contents without NULL terminator
    CharBufPtr buffer = getFileAsBuffer(filename, false);

    // Bind the program data into OpenGL
    GlobalOpenGL_debugAssertNoErrors();

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
        error += filename + "(" + intToStr(errPos) + "): \n\n";
        error += std::string(reinterpret_cast<const char*>(errString));

        // Throw exception
        //throw std::logic_error(error);
        std::cerr << error << std::endl;
	}

    // Return the new program
    return programID;
}

#endif // RADIANT_USE_GLSL

// Get the path of a GL program file
std::string GLProgramFactory::getGLProgramPath(const std::string& progName)
{
    // Determine the root path of the GL programs
#if defined(POSIX) && defined (PKGDATADIR)
    std::string glProgRoot = std::string(PKGDATADIR) + "/";
#else
    std::string glProgRoot = GlobalRegistry().get("user/paths/appPath");
#endif

    // Append the requested filename with the "gl/" directory.
    return glProgRoot + "gl/" + progName;
}

} // namespace render
