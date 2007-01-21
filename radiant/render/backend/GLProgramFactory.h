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
};

}

#endif /*GLPROGAMFACTORY_H_*/
