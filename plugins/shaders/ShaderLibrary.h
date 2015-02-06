#pragma once

#include <string>
#include <map>
#include "CShader.h"

namespace shaders 
{

class ShaderLibrary
{
	// The shader definitions act as precursor for a real shader
	// These are referenced by name.
	ShaderDefinitionMap _definitions;

	typedef std::map<std::string, CShaderPtr, ShaderNameCompareFunctor> ShaderMap;

	ShaderMap _shaders;

public:

	/* greebo: Add a shader definition to the internal list
	 * @returns: FALSE, if such a name already exists, TRUE otherwise
	 */
	bool addDefinition(const std::string& name, const ShaderDefinition& def);

	/* greebo: Trys to lookup the named shader definition and returns
	 * its reference. Always returns a valid reference.
	 */
	ShaderDefinition& getDefinition(const std::string& name);

	/** 
	 * Returns true if the given shader definition exists.
	 */
	bool definitionExists(const std::string& name) const;

	/* greebo: Clears out the stored shader definitions
	 */
	void clear();

	// Get the number of known shaders
	std::size_t getNumDefinitions();

	/* greebo: Retrieves the shader with the given name.
	 *
	 * @returns: the according CShaderPtr, this may also
	 * be a pointer to a dummy shader (shader not found)
	 */
	CShaderPtr findShader(const std::string& name);

	void foreachShaderName(const ShaderNameCallback& callback);

	// Traverse the library using the given functor
	void foreachShader(const std::function<void(const CShaderPtr&)>& func);
};
typedef std::shared_ptr<ShaderLibrary> ShaderLibraryPtr;

} // namespace shaders
