#include "ShaderLibrary.h"

#include <iostream>
#include <utility>
#include "ShaderTemplate.h"
#include "Doom3ShaderSystem.h"
#include "textures/FileLoader.h"
#include "parser/DefTokeniser.h"


namespace shaders {

ShaderLibrary::ShaderLibrary()
{
	_publicIterator = _shaders.begin();
}
	
// Insert into the definitions map, if not already present
bool ShaderLibrary::addDefinition(const std::string& name, 
								  const ShaderDefinition& def) 
{
	ShaderDefinitionMap::iterator i = _definitions.find(name);
	
	if (i != _definitions.end()) {
		// Return the definition
		return false;
	}
	else {
		_definitions.insert(std::make_pair(name, def));
		return true;
	}
}

ShaderDefinition& ShaderLibrary::getDefinition(const std::string& name) {
	// Try to lookup the named definition
	ShaderDefinitionMap::iterator i = _definitions.find(name);
	
	if (i != _definitions.end()) {
		// Return the definition
		return i->second;
	}
	else {
		globalErrorStream() << "Definition not found: " << name << "\n";
		
		// Create an empty template with this name
		ShaderTemplatePtr shaderTemplate(new ShaderTemplate(name));
				
		// Take this empty shadertemplate and create a ShaderDefinition
		ShaderDefinition def(shaderTemplate, "");
				
		// Insert the shader definition and set the iterator to it
		i = _definitions.insert(ShaderDefinitionMap::value_type(name, def)).first;
		
		return i->second;
	}
}

ShaderPtr ShaderLibrary::findShader(const std::string& name) {
	// Try to lookup the shader in the active shaders list
	ShaderMap::iterator i = _shaders.find(name);
	
	if (i != _shaders.end()) {
		// A shader has been found, return its pointer
		return i->second;
	}
	else {
		// No shader has been found, retrieve its definition (may also be a dummy def)
		ShaderDefinition& def = getDefinition(name);
		
		// Construct a new shader object with this def and insert it into the map
		ShaderPtr shader(new CShader(name, def));
		
		_shaders[name] = shader;
		
		return shader;
	}
}

void ShaderLibrary::clear() {
	_shaders.clear();
	_definitions.clear();
}

ShaderLibrary::iterator& ShaderLibrary::getIterator() {
	return _publicIterator;
}

void ShaderLibrary::incrementIterator() {
	if (_publicIterator != end()) {
		++_publicIterator;
	}
}

ShaderLibrary::iterator ShaderLibrary::begin() {
	return _shaders.begin();
}

ShaderLibrary::iterator ShaderLibrary::end() {
	return _shaders.end();
}

void ShaderLibrary::foreachShaderName(const ShaderNameCallback& callback) {	
	for (ShaderDefinitionMap::const_iterator i = _definitions.begin(); 
		 i != _definitions.end(); 
		 ++i) 
	{
		callback(i->first.c_str());
	}
}

TexturePtr ShaderLibrary::loadTextureFromFile(const std::string& filename, const std::string& moduleNames) {

	// Get the binding (i.e. load the texture)
	TexturePtr texture = GetTextureManager().getBinding(filename, moduleNames);

	return texture;
}

void ShaderLibrary::foreachShader(ShaderVisitor& visitor) {
	for (ShaderMap::iterator i = _shaders.begin(); i != _shaders.end(); ++i) {
		visitor.visit(i->second);
	}
}

} // namespace shaders
