#ifndef SOUNDSHADER_H_
#define SOUNDSHADER_H_

#include "isound.h"

#include <boost/shared_ptr.hpp>

namespace sound {

/**
 * Representation of a single sound shader.
 */
class SoundShader : 
	public ISoundShader
{
	// Name of the shader
	std::string _name;

	// The raw unparsed definition block
	std::string _blockContents;
	
	SoundFileList _soundFiles;

	// min and max radii of the shader
	SoundRadii _soundRadii;

	// Whether the raw definition block has been dissected already
	bool _parsed;
	
public:

	/**
	 * Constructor.
	 */
	SoundShader(const std::string& name, const std::string& blockContents) : 
		_name(name), 
		_blockContents(blockContents),
		_parsed(false)
	{}

	/**
	 * Return the min and max radii of the shader
	 */
	SoundRadii getRadii() {
		if (!_parsed) {
			// Evil const_cast to allow parsing on the fly
			const_cast<SoundShader*>(this)->parseDefinition();
		}
		return _soundRadii;
	}

	/**
	 * Return the name of the shader.
	 */
	std::string getName() {
		return _name;
	}

	/** 
	 * greebo: Returns the list of soundfiles
	 */
	SoundFileList getSoundFileList() {
		if (!_parsed) parseDefinition();
		return _soundFiles;
	}

private:
	// Parses the definition block
	void parseDefinition();
};

/**
 * Shared pointer type.
 */
typedef boost::shared_ptr<SoundShader> SoundShaderPtr;

}

#endif /*SOUNDSHADER_H_*/
