#include "SoundManager.h"

#include <iostream>

namespace sound
{

// Constructor
SoundManager::SoundManager()
{
	_shaders["first"] = ShaderPtr(new SoundShader("first"));
	_shaders["second"] = ShaderPtr(new SoundShader("second"));
	_shaders["third"] = ShaderPtr(new SoundShader("third"));
}

// Enumerate shaders
void SoundManager::forEachShader(SoundShaderVisitor visitor) const {
	for (ShaderMap::const_iterator i = _shaders.begin();
		 i != _shaders.end();
		 ++i)
	{
		visitor(*(i->second));	
	}	
}

}
