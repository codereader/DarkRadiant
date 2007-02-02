#include "ShaderLibrary.h"

#include <iostream>

ShaderLibrary::ShaderLibrary()
{
	std::cout << "ShaderLibrary initialised.\n";
}

ShaderLibrary::~ShaderLibrary() {
	std::cout << "ShaderLibrary shutdown.\n";
}
	
// Insert into the definitions map, if not already present
bool ShaderLibrary::addDefinition(const std::string& name, 
								  ShaderDefinition& def) 
{
	return _definitions.insert(ShaderDefinitionMap::value_type(name, def)).second;
}
