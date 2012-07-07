#include "ShaderLibrary.h"

#include <iostream>
#include <utility>
#include "itextstream.h"
#include "ShaderTemplate.h"
#include "Doom3ShaderSystem.h"
#include "textures/ImageFileLoader.h"
#include "parser/DefTokeniser.h"

namespace shaders {

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
	ImagePtr img = ImageFileLoader::imageFromVFS(name);

	if (img != NULL)
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

void ShaderLibrary::clear() {
	_shaders.clear();
	_definitions.clear();
}

std::size_t ShaderLibrary::getNumShaders() {
	return _definitions.size();
}

void ShaderLibrary::foreachShaderName(const ShaderNameCallback& callback)
{
	for (ShaderDefinitionMap::const_iterator i = _definitions.begin();
		 i != _definitions.end();
		 ++i)
	{
		callback(i->first);
	}
}

TexturePtr ShaderLibrary::loadTextureFromFile(const std::string& filename,
                                                const std::string& moduleNames)
{
	// Get the binding (i.e. load the texture)
	TexturePtr texture = GetTextureManager().getBinding(filename, moduleNames);

	return texture;
}

void ShaderLibrary::foreachShader(ShaderVisitor& visitor)
{
	for (ShaderMap::const_iterator i = _shaders.begin(); i != _shaders.end(); ++i)
	{
		visitor.visit(i->second);
	}
}

void ShaderLibrary::realiseLighting()
{
	// First unrealise the lighting of all shaders
	for (ShaderMap::const_iterator i = _shaders.begin(); i != _shaders.end(); ++i)
	{
		i->second->realiseLighting();
	}
}

void ShaderLibrary::unrealiseLighting()
{
	for (ShaderMap::const_iterator i = _shaders.begin(); i != _shaders.end(); ++i)
	{
		i->second->unrealiseLighting();
	}
}

} // namespace shaders
