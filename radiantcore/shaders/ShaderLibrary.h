#pragma once

#include <string>
#include <map>
#include <memory>
#include "CShader.h"

namespace shaders
{

class ShaderLibrary
{
	// The shader definitions act as precursor for a real shader
	// These are referenced by name.
	ShaderDefinitionMap _definitions;

	typedef std::map<std::string, CShaderPtr, string::ILess> ShaderMap;
    ShaderMap _shaders;

    std::unique_ptr<ShaderDefinition> _emptyDefinition;

public:

	/* greebo: Add a shader definition to the internal list
	 * @returns: FALSE, if such a name already exists, TRUE otherwise
	 */
	bool addDefinition(const std::string& name, const ShaderDefinition& def);

	/* greebo: Trys to lookup the named shader definition and returns
	 * its reference. Always returns a valid reference.
	 */
	ShaderDefinition& getDefinition(const std::string& name);

    // Updates the stored definition in the library with the given one
    void replaceDefinition(const std::string& name, const ShaderDefinition& def);

	/**
	 * Returns true if the given shader definition exists.
	 */
	bool definitionExists(const std::string& name) const;

    // Copies the given definition, original name must be present, new name must not conflict
    void copyDefinition(const std::string& nameOfOriginal, const std::string& nameOfCopy);

    // Renames the definition oldName => newName. oldName must be present, newName must not be present
    void renameDefinition(const std::string& oldName, const std::string& newName);

    // Removes the named definition. The name must be present in the library.
    void removeDefinition(const std::string& name);

    // Returns an empty definition, just enough to construct a shader from it
    ShaderDefinition& getEmptyDefinition();

	/* greebo: Clears out all internal containers (definitions, tables, shaders)
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
