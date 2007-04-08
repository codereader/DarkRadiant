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
};

/**
 * Shared pointer type.
 */
typedef boost::shared_ptr<SoundShader> ShaderPtr;

}

#endif /*SOUNDSHADER_H_*/
