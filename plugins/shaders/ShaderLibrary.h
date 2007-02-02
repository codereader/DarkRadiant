#ifndef SHADERLIBRARY_H_
#define SHADERLIBRARY_H_

#include <string>
#include <map>
#include "CShader.h"

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
	bool addDefinition(const std::string& name, ShaderDefinition& def);

}; // class ShaderLibrary

#endif /*SHADERLIBRARY_H_*/
