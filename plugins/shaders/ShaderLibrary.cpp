#include "ShaderLibrary.h"

#include <iostream>
#include "ShaderTemplate.h"

namespace shaders {

ShaderLibrary::ShaderLibrary()
{
	std::cout << "ShaderLibrary initialised.\n";
	_publicIterator = _shaders.begin();
}

ShaderLibrary::~ShaderLibrary() {
	std::cout << "ShaderLibrary shutdown.\n";
}
	
// Insert into the definitions map, if not already present
bool ShaderLibrary::addDefinition(const std::string& name, 
								  const ShaderDefinition& def) 
{
	return _definitions.insert(ShaderDefinitionMap::value_type(name, def)).second;
}

ShaderDefinition& ShaderLibrary::getDefinition(const std::string& name) {
	// Try to lookup the named definition
	ShaderDefinitionMap::iterator i = _definitions.find(name);
	
	if (i != _definitions.end()) {
		// Return the definition
		return i->second;
	}
	else {
		std::cout << "Definition not found: " << name.c_str() << "\n";
		
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
		_publicIterator++;
	}
}

ShaderLibrary::iterator ShaderLibrary::begin() {
	return _shaders.begin();
}

ShaderLibrary::iterator ShaderLibrary::end() {
	return _shaders.end();
}

} // namespace shaders
