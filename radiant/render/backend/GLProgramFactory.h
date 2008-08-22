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

private:

	// Private constructor, populates internal map
	GLProgramFactory();

	// Static instance owner
	static GLProgramFactory& getInstance();
	
public:

	/**
	 * Static method to return the GLProgram instance corresponding to the given
	 * text name.
	 */
	static GLProgramPtr getProgram(const std::string& name);
	
	/**
	 * Static realise method, called by the ShaderCache when the GLPrograms
	 * need to be initialised.
	 */
	static void realise();
	
	/**
	 * Static unrealise method, called when the GLPrograms should be destroyed.
	 */
	static void unrealise();
	
	/**
     * Create an ARB GL Program by calling glProgramStringARB with the contents of
	 * a file.
     */
	static void createARBProgram(const std::string& filename, GLenum type);

    /**
     * Convenience method to return the full path of a given GL program file on
     * disk, taking account of platform-dependent differences.
     *
     * @param progName
     * The filename of the GL program without directory path (e.g.
     * "interaction_fp.arb").
     */
    static std::string getGLProgramPath(const std::string& progName);
};

}

#endif /*GLPROGAMFACTORY_H_*/
