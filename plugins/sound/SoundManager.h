#ifndef SOUNDMANAGER_H_
#define SOUNDMANAGER_H_

#include "SoundShader.h"
#include "SoundPlayer.h"

#include "isound.h"
#include "parser/DefTokeniser.h"

#include <map>

namespace sound {

/**
 * SoundManager implementing class.
 */
class SoundManager : 
	public ISoundManager
{
	// Map of named sound shaders
	typedef std::map<std::string, ShaderPtr> ShaderMap;
	ShaderMap _shaders;
	
	SoundShader _emptyShader;
	
	// Parse a single sound shader from the given token stream
	void parseSoundShader(parser::DefTokeniser& tok);
	
	// The helper class for playing the sounds
	SoundPlayer _soundPlayer;
	
public:
	/**
	 * Main constructor.
	 */
	SoundManager();
	
	/**
	 * Enumerate sound shaders.
	 */
	void forEachShader(SoundShaderVisitor visitor) const;
	
	/** greebo: Returns the soundshader with the name <shaderName>   
	 */
	const ISoundShader& getSoundShader(const std::string& shaderName); 
	
	/** greebo: Plays the sound file. Tries to resolve the filename's
	 * 			extension by appending .ogg or .wav and such.
	 */
	virtual bool playSound(const std::string& fileName);
	
	/** greebo: Stops the playback immediately.
	 */
	virtual void stopSound();
	
	/**
	 * Parse the contents of the given string as a .sndshd file, adding all
	 * contained shaders to the shader map.
	 */
	void parseShadersFrom(std::istream& contents);
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<SoundManager> SoundManagerPtr;

}

#endif /*SOUNDMANAGER_H_*/
