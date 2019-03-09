#include "ShaderLibrary.h"

#include <iostream>
#include <utility>
#include "iimage.h"
#include "itextstream.h"
#include "ShaderTemplate.h"

namespace shaders 
{

// Insert into the definitions map, if not already present
bool ShaderLibrary::addDefinition(const std::string& name,
								  const ShaderDefinition& def)
{
	std::pair<ShaderDefinitionMap::iterator, bool> result = _definitions.insert(
		ShaderDefinitionMap::value_type(name, def)
	);

	return result.second;
}

ShaderDefinition& ShaderLibrary::getDefinition(const std::string& name)
{
	// Try to lookup the named definition
	ShaderDefinitionMap::iterator i = _definitions.find(name);

	if (i != _definitions.end())
    {
		// Return the definition
		return i->second;
	}

	// The shader definition hasn't been found, let's check if the name
	// refers to a file in the VFS
	ImagePtr img = GlobalImageLoader().imageFromVFS(name);

	if (img)
	{
		// Create a new template with this name
		ShaderTemplatePtr shaderTemplate(new ShaderTemplate(name, ""));

		// Add a diffuse layer to that template, using the given texture path
		MapExpressionPtr imgExpr(new ImageExpression(name));
		shaderTemplate->addLayer(ShaderLayer::DIFFUSE, imgExpr);

		// Take this empty shadertemplate and create a ShaderDefinition
		ShaderDefinition def(shaderTemplate, "");

		// Insert the shader definition and set the iterator to it
		i = _definitions.insert(ShaderDefinitionMap::value_type(name, def)).first;

		return i->second;
	}
	else
	{
        rWarning() << "[shaders] ShaderLibrary: definition not found: "
			<< name << std::endl;

		// Create an empty template with this name
		ShaderTemplatePtr shaderTemplate(new ShaderTemplate(name, ""));

		// Take this empty shadertemplate and create a ShaderDefinition
		ShaderDefinition def(shaderTemplate, "");

		// Insert the shader definition and set the iterator to it
		i = _definitions.insert(ShaderDefinitionMap::value_type(name, def)).first;

		return i->second;
	}
}

bool ShaderLibrary::definitionExists(const std::string& name) const
{
	ShaderDefinitionMap::const_iterator i = _definitions.find(name);

	return i != _definitions.end();
}

CShaderPtr ShaderLibrary::findShader(const std::string& name)
{
	// Try to lookup the shader in the active shaders list
	ShaderMap::iterator i = _shaders.find(name);

	if (i != _shaders.end())
    {
		// A shader has been found, return its pointer
		return i->second;
	}
	else
    {
        // No shader has been found, retrieve its definition (may also be a
        // dummy def)
        ShaderDefinition& def = getDefinition(name);

        // Construct a new shader object with this def and insert it into the
        // map
        CShaderPtr shader(new CShader(name, def));

		_shaders[name] = shader;

		return shader;
	}
}

void ShaderLibrary::clear()
{
	_shaders.clear();
	_definitions.clear();
    _tables.clear();
}

std::size_t ShaderLibrary::getNumDefinitions()
{
	return _definitions.size();
}

void ShaderLibrary::foreachShaderName(const ShaderNameCallback& callback)
{
    for (const ShaderDefinitionMap::value_type& pair : _definitions)
	{
		callback(pair.first);
	}
}

void ShaderLibrary::foreachShader(const std::function<void(const CShaderPtr&)>& func)
{
	for (const ShaderMap::value_type& pair : _shaders)
	{
        func(pair.second);
	}
}

TableDefinitionPtr ShaderLibrary::getTableForName(const std::string& name)
{
    TableDefinitions::const_iterator i = _tables.find(name);

    return i != _tables.end() ? i->second : TableDefinitionPtr();
}

bool ShaderLibrary::addTableDefinition(const TableDefinitionPtr& def)
{
    std::pair<TableDefinitions::iterator, bool> result = _tables.insert(
        TableDefinitions::value_type(def->getName(), def));

    return result.second;
}

} // namespace shaders
