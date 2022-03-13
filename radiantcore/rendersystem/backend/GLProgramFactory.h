#pragma once

#include <map>
#include <memory>
#include "igl.h"

namespace render
{

/**
 * GLProgram shared pointer type.
 */
typedef std::shared_ptr<GLProgram> GLProgramPtr;

enum class ShaderProgram
{
    DepthFill,
    DepthFillAlpha,
    Interaction,
    CubeMap,
};

/**
 * Factory class reponsible for creating GLProgam instances addressed by name.
 */
class GLProgramFactory
{
	// Internal map of string names to GLProgram subclasses
	typedef std::map<ShaderProgram, GLProgramPtr> ProgramMap;
	ProgramMap _builtInPrograms;

    // Game-specific programs are referenced as pair
    typedef std::map<std::pair<std::string, std::string>, GLProgramPtr> GameProgramMap;
    GameProgramMap _gamePrograms;

    // Using GLSL flag
    bool _usingGLSL;

public:
    // Constructor, populates internal map
    GLProgramFactory();

    /**
     * Get the named built-in GL program.
     *
     * Returns a raw pointer which is owned by the GLProgramFactory and should
     * never be deleted.
     */
	GLProgram* getBuiltInProgram(ShaderProgram builtInProgram);

    /**
     * Gets or creates the GL program for the given V/F program filenames.
     * The programs will be loaded from the game's glprogs/ folder
     * as in the idTech4 engine.
     *
     * The returned pointer is always non-null, on failures a
     * std::runtime_error will be thrown.
     */
    GLProgram* getProgram(const std::string& vertexProgramFilename,
                          const std::string& fragmentProgramFilename);

    /// Construct and initialise the GLPrograms
	void realise();

    /// Release and destroy GLProgram resources
	void unrealise();

    /**
     * \brief
     * Create a GLSL shader object using the given source files.
     *
     * \param vFile
     * Relative filename for the vertex shader code.
     *
     * \param fFile
     * Relative filename for the fragment shader code
     *
     * \return
     * The program object id for subsequent binding with glUseProgram(). The
     * program will be compiled, but not linked. The calling code should bind
     * any necessary attributes and then call glLinkProgram() to finalise the
     * link.
     */
    static GLuint createGLSLProgram(const std::string& vFile, const std::string& fFile);
};

} // namespace
