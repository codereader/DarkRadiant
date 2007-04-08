#ifndef SOUNDMANAGER_H_
#define SOUNDMANAGER_H_

#include "isound.h"

namespace sound
{

/**
 * SoundManager implementing class.
 */
class SoundManager
: public ISoundManager
{
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
	
	// Enumerate sound shaders
	void forEachShader(SoundShaderVisitor visitor) {
				
	}	
};

}

#endif /*SOUNDMANAGER_H_*/
