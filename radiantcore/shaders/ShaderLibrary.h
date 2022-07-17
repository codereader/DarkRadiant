#pragma once

#include <string>
#include <map>
#include <memory>
#include "CShader.h"
#include "string/string.h"

namespace shaders
{

class ShaderLibrary
{
	typedef std::map<std::string, CShaderPtr, string::ILess> ShaderMap;
    ShaderMap _shaders;

public:

	/* greebo: Trys to lookup the named shader definition and returns
	 * its reference. Always returns a valid reference.
	 */
    std::shared_ptr<ShaderTemplate> getTemplate(const std::string& name);

	/**
	 * Returns true if the given shader definition exists.
	 */
	bool definitionExists(const std::string& name) const;

    // Copies the given definition, original name must be present, new name must not conflict
    void copyDefinition(const std::string& nameOfOriginal, const std::string& nameOfCopy);

    // Renames the definition oldName => newName. oldName must be present, newName must not be present
    bool renameDefinition(const std::string& oldName, const std::string& newName);

    // Removes the named definition. The name must be present in the library.
    void removeDefinition(const std::string& name);

	/* greebo: Clears out all internal containers (definitions, tables, shaders)
	 */
	void clear();

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
