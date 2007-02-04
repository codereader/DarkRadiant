#ifndef SHADERLIBRARY_H_
#define SHADERLIBRARY_H_

#include <string>
#include <map>
#include "CShader.h"

namespace shaders {

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
	
	/* greebo: Clears out the stored shader definitions
	 */
	void clear();
	
	/* greebo: Retrieves the shader with the given name.
	 * 
	 * @returns: the according ShaderPtr, this may also
	 * be a pointer to a dummy shader (shader not found) 
	 */
	ShaderPtr findShader(const std::string& name);

}; // class ShaderLibrary

typedef boost::shared_ptr<ShaderLibrary> ShaderLibraryPtr;

} // namespace shaders

#endif /*SHADERLIBRARY_H_*/
