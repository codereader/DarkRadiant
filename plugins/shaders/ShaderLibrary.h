#ifndef SHADERLIBRARY_H_
#define SHADERLIBRARY_H_

#include <string>
#include <map>
#include "CShader.h"

namespace shader {

class ShaderLibrary
{
	// The shader definitions act as precursor for a real shader
	// These are referenced by name. 
	ShaderDefinitionMap _definitions;
	
	typedef std::map<std::string, ShaderPtr> ShaderMap;
	typedef ShaderMap::iterator iterator;
	
	ShaderMap _shaders;

public:
	// Constructor
	ShaderLibrary();
	
	// Destructor
	~ShaderLibrary();
	
	/* greebo: Add a shader definition to the internal list 
	 * @returns: FALSE, if such a name already exists, TRUE otherwise
	 */
	bool addDefinition(const std::string& name, const ShaderDefinition& def);
	
	/* greebo: Trys to lookup the named shader definition and returns
	 * its reference. Throws a MissingShaderDefException, if the
	 * name is not found in the map. 
	 */
	ShaderDefinition& getDefinition(const std::string& name);

}; // class ShaderLibrary

} // namespace shader

#endif /*SHADERLIBRARY_H_*/
