#include "GLProgramFactory.h"
#include "glprogram/ARBBumpProgram.h"
#include "glprogram/ARBDepthFillProgram.h"

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

}
