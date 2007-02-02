#include "ShaderLibrary.h"

#include <iostream>
#include "ShaderTemplate.h"

namespace shader {

ShaderLibrary::ShaderLibrary()
{
	std::cout << "ShaderLibrary initialised.\n";
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
		// Create an empty template with this name
		ShaderTemplatePtr shaderTemplate(new ShaderTemplate(name));
				
		// Take this empty shadertemplate and create a ShaderDefinition
		ShaderDefinition def(shaderTemplate, "");
				
		// Insert the shader definition and set the iterator to it
		i = _definitions.insert(ShaderDefinitionMap::value_type(name, def)).first;
		
		return i->second;
	}
}

} // namespace shader
