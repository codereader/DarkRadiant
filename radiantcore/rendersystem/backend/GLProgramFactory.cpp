#include "GLProgramFactory.h"

#include "glprogram/GLSLDepthFillProgram.h"
#include "glprogram/GLSLDepthFillAlphaProgram.h"
#include "glprogram/GLSLCubeMapProgram.h"
#include "glprogram/GLSLBumpProgram.h"
#include "glprogram/GenericVFPProgram.h"

#include "itextstream.h"
#include "iregistry.h"
#include "imodule.h"
#include "os/file.h"
#include "string/convert.h"
#include "debugging/debugging.h"
#include "debugging/gl.h"

#include <fstream>

namespace render
{

// Constructor, populates map with GLProgram instances
GLProgramFactory::GLProgramFactory()
{
    _builtInPrograms[ShaderProgram::DepthFill] = std::make_shared<GLSLDepthFillProgram>();
    _builtInPrograms[ShaderProgram::DepthFillAlpha] = std::make_shared<GLSLDepthFillAlphaProgram>();
    _builtInPrograms[ShaderProgram::Interaction] = std::make_shared<GLSLBumpProgram>();
    _builtInPrograms[ShaderProgram::CubeMap] = std::make_shared<GLSLCubeMapProgram>();
}

GLProgram* GLProgramFactory::getBuiltInProgram(ShaderProgram builtInProgram)
{
	// Lookup the program, if not found throw an exception
	auto i = _builtInPrograms.find(builtInProgram);

    if (i != _builtInPrograms.end())
    {
        return i->second.get();
    }

	throw std::runtime_error("GLProgramFactory: failed to find program " + string::to_string((int)builtInProgram));
}

GLProgram* GLProgramFactory::getProgram(const std::string& vertexProgramFilename,
                                        const std::string& fragmentProgramFilename)
{
    std::pair<std::string, std::string> filePair =
        std::make_pair(vertexProgramFilename, fragmentProgramFilename);

    // Check existing programs
    GameProgramMap::iterator i = _gamePrograms.find(filePair);

    if (i != _gamePrograms.end())
    {
        return i->second.get();
    }

    std::pair<GameProgramMap::iterator, bool> result = _gamePrograms.insert(
        std::make_pair(filePair, std::make_shared<GenericVFPProgram>(vertexProgramFilename, fragmentProgramFilename)));

    return result.first->second.get();
}

// Realise the program factory.
void GLProgramFactory::realise()
{
	// Realise each GLProgram in the map
	for (ProgramMap::value_type& pair : _builtInPrograms)
	{
		pair.second->create();
	}
}

// Unrealise the program factory.
void GLProgramFactory::unrealise() 
{
	// Destroy each GLProgram in the map
    for (ProgramMap::value_type& pair : _builtInPrograms)
	{
		pair.second->destroy();
	}
}

// Get file as a char buffer
GLProgramFactory::CharBufPtr
GLProgramFactory::getFileAsBuffer(const std::string& filename,
                                  bool nullTerminated)
{
    // Get absolute path from filename
    std::string absFileName = getBuiltInGLProgramPath(filename);

    // Open the file
	std::size_t size = os::getFileSize(absFileName);
	std::ifstream file(absFileName.c_str());

    // Throw an exception if the file could not be found
	if (!file.is_open())
    {
        throw std::runtime_error(
            "GLProgramFactory: failed to open file: " + absFileName
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

    // Output to stream
    rMessage() << "[renderer] GLSL program "
              << (validStatus == GL_TRUE ? "IS " : "IS NOT ") << "valid.\n";
    rMessage() << "Info:\n" << validLog << std::endl;

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
    debug::assertNoGlErrors();

    // Compile the shaders
    glCompileShader(vertexShader);
    assertShaderCompiled(vertexShader);

    glCompileShader(fragmentShader);
    assertShaderCompiled(fragmentShader);

    debug::assertNoGlErrors();

    // Attach and link the program object itself
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    debug::assertNoGlErrors();

    glLinkProgram(program);

    // Check the link status and throw an exception if it failed
    assertProgramLinked(program);

    // Return the linked program
    return program;
}

// Get the path of a GL program file
std::string GLProgramFactory::getBuiltInGLProgramPath(const std::string& progName)
{
    // Append the requested filename with the "gl/" directory.
    return module::GlobalModuleRegistry()
            .getApplicationContext()
                .getRuntimeDataPath()
                    + "gl/" + progName;
}

} // namespace render
