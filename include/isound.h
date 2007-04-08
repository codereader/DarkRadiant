#ifndef ISOUND_H_
#define ISOUND_H_

#include "generic/constant.h"
#include "modulesystem.h"

#include <string>
#include <boost/function.hpp>

/**
 * Representation of a single sound or sound shader.
 */
struct ISoundShader {

	/**
	 * Get the name of the shader.
	 */
	virtual std::string getName() const = 0;
	
};

/**
 * Sound shader visitor function typedef.
 */
typedef boost::function< void (const ISoundShader&) > SoundShaderVisitor;

/**
 * Sound manager interface.
 */
struct ISoundManager {
	
	/* Radiant module stuff */
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "soundmanager");
	
	/**
	 * Enumerate each of the sound shaders.
	 */
	void forEachShader(SoundShaderVisitor visitor);
	
};

/* Module types */

typedef GlobalModule<ISoundManager> GlobalSoundManagerModule;
typedef GlobalModuleRef<ISoundManager> GlobalSoundManagerModuleRef;

inline ISoundManager& GlobalSoundManager() {
	return GlobalSoundManagerModule::getTable();	
}

#endif /*ISOUND_H_*/
