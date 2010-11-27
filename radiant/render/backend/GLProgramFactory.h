#ifndef GLPROGAMFACTORY_H_
#define GLPROGAMFACTORY_H_

#include "iglrender.h"

#include <map>
#include <boost/shared_ptr.hpp>

namespace render
{

/**
 * GLProgram shared pointer type.
 */
typedef boost::shared_ptr<GLProgram> GLProgramPtr;

/**
 * Factory class reponsible for creating GLProgam instances addressed by name.
 */
class GLProgramFactory
{
	// Internal map of string names to GLProgram subclasses
	typedef std::map<std::string, GLProgramPtr> ProgramMap;
	ProgramMap _map;

    // Using GLSL flag
    bool _usingGLSL;

private:

	// Private constructor, populates internal map
	GLProgramFactory();

    /*
     * Convenience method to return the full path of a given GL program file on
     * disk, taking account of platform-dependent differences.
     */
    static std::string getGLProgramPath(const std::string& progName);

    // Get a vector of chars containing file contents, with optional
    // NULL-termination
    typedef boost::shared_ptr<std::vector<char> > CharBufPtr;
    static CharBufPtr getFileAsBuffer(const std::string& filename,
                                      bool nullTerminated);

    // Get the program info log as a string
    static std::string getProgramInfoLog(GLuint program);

    // Check the status of a shader, and throw exception with the info log if it
    // is not valid
    static void assertShaderCompiled(GLuint shader);

    // Check the program has linked, throwing exception if failed
    static void assertProgramLinked(GLuint program);

public:

    /// Get the static factory instance
	static GLProgramFactory& instance();

    /**
     * Get the named GL program.
     *
     * Returns a raw pointer which is owned by the GLProgramFactory and should
     * never be deleted.
     */
	GLProgram* getProgram(const std::string& name);

    /// Set whether to use GLSL or ARB programs
    void setUsingGLSL(bool useGLSL);

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
    static GLuint createGLSLProgram(const std::string& vFile,
                                    const std::string& fFile);

	/**
     * Create a GL Program from the contents of a file.
     *
     * \param filename
     * The filename of the GL program without directory path (e.g.
     * "interaction_fp.arb").
     *
     * \param type
     * The type of the program to create, either GL_VERTEX_PROGRAM_ARB or
     * GL_FRAGMENT_PROGRAM_ARB.
     *
     * \return
     * The GL program ID to be used for subsequent binding.
     */
    static GLuint createARBProgram(const std::string& filename, GLenum type);

};

}

#endif /*GLPROGAMFACTORY_H_*/
