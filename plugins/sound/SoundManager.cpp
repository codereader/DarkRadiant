#include "SoundManager.h"

#include <iostream>

namespace sound
{

SoundManager::SoundManager()
{
	_shaders["first"] = ShaderPtr(new SoundShader("first"));
	_shaders["second"] = ShaderPtr(new SoundShader("second"));
	_shaders["third"] = ShaderPtr(new SoundShader("third"));
}

}
