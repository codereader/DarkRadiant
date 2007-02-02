#include "ShaderLibrary.h"

ShaderLibrary::ShaderLibrary()
{}

ShaderLibrary::~ShaderLibrary() {
	
}
	
ShaderLibrary::iterator ShaderLibrary::begin() {
	return _shaders.begin();
}

ShaderLibrary::iterator ShaderLibrary::end()  {
	return _shaders.end();
}

ShaderLibrary::iterator ShaderLibrary::find(const std::string& name)  {
	return _shaders.find(name);
}
