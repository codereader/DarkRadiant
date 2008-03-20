#ifndef ISOUND_H_
#define ISOUND_H_

#include "imodule.h"
#include <vector>
#include <boost/function.hpp>

// A list of sound files associated to a shader
typedef std::vector<std::string> SoundFileList;

// The min and max radii of a sound shader
class SoundRadii {
	float minRad, maxRad;
	public:
	//set sound radii either in metres or in inch on initialization might cause a conversion
	SoundRadii (float min = 0, float max = 0, bool inMetres = false) {
		if (inMetres) {
			minRad = min/0.0254f;
			maxRad = max/0.0254f;
		}
		else {
			minRad = min;
			maxRad = max;
		}
	}
	// set the sound radii in metres or in inch, might cause a conversion
	inline void setMin (float min, bool inMetres = false) {
		if (inMetres) minRad = min/0.0254f;
		else minRad = min;
	}
	inline void setMax (float max, bool inMetres = false) {
		if (inMetres) maxRad = max/0.0254f;
		else maxRad = max;
	}
	inline float getMin () const { return minRad; }
	inline float getMax () const { return maxRad; }
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

const std::string MODULE_SOUNDMANAGER("SoundManager");

/**
 * Sound manager interface.
 */
class ISoundManager :
	public RegisterableModule
{
public:	
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

// Accessor method
inline ISoundManager& GlobalSoundManager() {
	// Cache the reference locally
	static ISoundManager& _soundManager(
		*boost::static_pointer_cast<ISoundManager>(
			module::GlobalModuleRegistry().getModule(MODULE_SOUNDMANAGER)
		)
	);
	return _soundManager;	
}

#endif /*ISOUND_H_*/
