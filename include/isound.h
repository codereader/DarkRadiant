#ifndef ISOUND_H_
#define ISOUND_H_

#include "generic/constant.h"
#include "modulesystem.h"

#include <string>
#include <vector>
#include <boost/function.hpp>

// A list of sound files associated to a shader
typedef std::vector<std::string> SoundFileList;

// The min and max radii of a sound shader
class SoundRadii {
	public:
	int minRad, maxRad;
	SoundRadii (int min = 0, int max = 0) : minRad(min), maxRad(max) {};
};

/**
 * Representation of a single sound or sound shader.
 */
struct ISoundShader {

	/**
	 * Get the name of the shader.
	 */
	virtual std::string getName() const = 0;

	/**
	 * Get the min and max radii of the shader.
	 */
	virtual SoundRadii getRadii() const = 0;
	
	/** greebo: Get the list of sound files associated to 
	 * 			this shader.
	 */
	virtual SoundFileList getSoundFileList() const = 0;
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
	virtual void forEachShader(SoundShaderVisitor visitor) const = 0;
	
	/** greebo: Tries to lookup the SoundShader with the given name,
	 * 			returns a soundshader with an empty name, if the lookup failed.
	 */
	virtual const ISoundShader& getSoundShader(const std::string& shaderName) = 0;
	
	/** greebo: Plays the given sound file (defined by its VFS path).
	 * 
	 * @returns: TRUE, if the sound file was found at the given VFS path, 
	 * 			 FALSE otherwise
	 */
	virtual bool playSound(const std::string& fileName) = 0; 
	
	/** greebo: Stops the currently played sound.
	 */
	virtual void stopSound() = 0;
};

/* Module types */

typedef GlobalModule<ISoundManager> GlobalSoundManagerModule;
typedef GlobalModuleRef<ISoundManager> GlobalSoundManagerModuleRef;

inline ISoundManager& GlobalSoundManager() {
	return GlobalSoundManagerModule::getTable();	
}

#endif /*ISOUND_H_*/
