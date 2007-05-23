#ifndef SOUNDSHADER_H_
#define SOUNDSHADER_H_

#include "isound.h"

#include <boost/shared_ptr.hpp>

namespace sound
{

/**
 * Representation of a single sound shader.
 */
class SoundShader
: public ISoundShader
{
	// Name of the shader
	std::string _name;
	
	SoundFileList _soundFiles;
	
public:

	/**
	 * Constructor.
	 */
	SoundShader(const std::string& name)
	: _name(name) 
	{ }

	/**
	 * Return the name of the shader.
	 */
	std::string getName() const {
		return _name;
	}
	
	/** greebo: Adds a sound file (VFS path) to this shader
	 * 			(used by the tokeniser to populate this class)
	 */
	void addSoundFile(const std::string& file) {
		_soundFiles.push_back(file);
	}
	
	/** greebo: Returns the list of soundfiles
	 */
	virtual SoundFileList getSoundFileList() const {
		return _soundFiles;
	}
};

/**
 * Shared pointer type.
 */
typedef boost::shared_ptr<SoundShader> ShaderPtr;

}

#endif /*SOUNDSHADER_H_*/
