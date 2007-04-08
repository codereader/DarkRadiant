#ifndef SOUNDMANAGER_H_
#define SOUNDMANAGER_H_

#include "SoundShader.h"

#include "isound.h"

#include <map>
#include <string>

namespace sound
{

/**
 * SoundManager implementing class.
 */
class SoundManager
: public ISoundManager
{
	// Map of named sound shaders
	typedef std::map<std::string, ShaderPtr> ShaderMap;
	ShaderMap _shaders;
	
public:

	/* Module typedefs */
	typedef ISoundManager Type;
	STRING_CONSTANT(Name, "*");
	
	/**
	 * Main constructor.
	 */
	SoundManager();
	
	/**
	 * Required getTable() function for SingletonModule<>
	 */
	SoundManager* getTable() {
		return this;
	}
	
	/**
	 * Enumerate sound shaders.
	 */
	void forEachShader(SoundShaderVisitor visitor) const;
};

}

#endif /*SOUNDMANAGER_H_*/
