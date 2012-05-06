#ifndef _SOUND_INTERFACE_H_
#define _SOUND_INTERFACE_H_

#include <boost/python.hpp>

#include "isound.h"
#include "iscript.h"

namespace script {

class ScriptSoundRadii
{
	SoundRadii _radii;
public:
	ScriptSoundRadii() :
		_radii(0,0,false)
	{}

	ScriptSoundRadii(const SoundRadii& radii) :
		_radii(radii)
	{}

	operator SoundRadii&() {
		return _radii;
	}

	void setMin(float min, bool inMetres)
	{
		_radii.setMin(min, inMetres);
	}

	void setMax (float max, bool inMetres)
	{
		_radii.setMax(max, inMetres);
	}

	float getMin(bool inMetres) const
	{
		return _radii.getMin(inMetres);
	}

	float getMax(bool inMetres) const
	{
		return _radii.getMax(inMetres);
	}
};

/**
 * This class represents a single sound shader for a script.
 */
class ScriptSoundShader
{
	ISoundShaderPtr _shader;

public:
	ScriptSoundShader(const ISoundShaderPtr& shader) :
		_shader(shader)
	{}

	operator ISoundShaderPtr&() {
		return _shader;
	}

	std::string getName()
	{
		return (_shader != NULL) ? _shader->getName() : "";
	}

	ScriptSoundRadii getRadii()
	{
		return (_shader != NULL) ? ScriptSoundRadii(_shader->getRadii()) : ScriptSoundRadii();
	}

	SoundFileList getSoundFileList()
	{
		return (_shader != NULL) ? _shader->getSoundFileList() : SoundFileList();
	}

	bool isNull() const {
		return _shader == NULL;
	}
};

/**
 * greebo: This class provides the script interface for the GlobalSoundManager module.
 */
class SoundManagerInterface :
	public IScriptInterface
{
public:
	// Wrapper methods for exposing the SoundManager interface
	ScriptSoundShader getSoundShader(const std::string& shaderName);
	bool playSound(const std::string& fileName);
	void stopSound();

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<SoundManagerInterface> SoundManagerInterfacePtr;

} // namespace script

#endif /* _SOUND_INTERFACE_H_ */
