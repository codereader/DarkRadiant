#ifndef SOUNDMANAGER_H_
#define SOUNDMANAGER_H_

#include "SoundShader.h"
#include "SoundPlayer.h"

#include "isound.h"

#include <map>
#include <string>

/* FORWARD DECLS */
namespace parser { class DefTokeniser; }

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
	
private:

	// Parse a single sound shader from the given token stream
	void parseSoundShader(parser::DefTokeniser& tok);
	
	// The helper class for playing the sounds
	SoundPlayer _soundPlayer;
	
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
	
	/** greebo: Plays the sound shader. If the sound shader has multiple
	 * 			possible sounds, a random one is chosen.
	 */
	virtual void playSoundShader(const ISoundShader& soundShader);
	
	/**
	 * Parse the contents of the given string as a .sndshd file, adding all
	 * contained shaders to the shader map.
	 */
	void parseShadersFrom(const std::string& contents);
};

}

#endif /*SOUNDMANAGER_H_*/
