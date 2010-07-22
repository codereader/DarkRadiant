#ifndef SOUNDMANAGER_H_
#define SOUNDMANAGER_H_

#include "SoundShader.h"
#include "SoundPlayer.h"

#include "isound.h"

#include <map>

namespace sound {

/**
 * SoundManager implementing class.
 */
class SoundManager : 
	public ISoundManager
{
public: /* TYPES */

	// Map of named sound shaders
	typedef std::map<std::string, SoundShaderPtr> ShaderMap;

private: /* FIELDS */

    // Master map of shaders
	mutable ShaderMap _shaders;
	
	SoundShaderPtr _emptyShader;
	
	// The helper class for playing the sounds
	boost::shared_ptr<SoundPlayer> _soundPlayer;

    // Did we populate from the filesystem yet?
    mutable bool _shadersLoaded;
	
private: /* METHODS */

    void loadShadersFromFilesystem() const;
    void ensureShadersLoaded() const;

public:
	/**
	 * Main constructor.
	 */
	SoundManager();
	
	/**
	 * Enumerate sound shaders.
	 */
	void forEachShader(SoundShaderVisitor& visitor) const;
	
	/** greebo: Returns the soundshader with the name <shaderName>   
	 */
	ISoundShaderPtr getSoundShader(const std::string& shaderName); 
	
	/** greebo: Plays the sound file. Tries to resolve the filename's
	 * 			extension by appending .ogg or .wav and such.
	 */
	virtual bool playSound(const std::string& fileName);
	
	/** greebo: Stops the playback immediately.
	 */
	virtual void stopSound();
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<SoundManager> SoundManagerPtr;

}

#endif /*SOUNDMANAGER_H_*/
