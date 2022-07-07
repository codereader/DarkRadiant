#pragma once

#include "isound.h"
#include "iscript.h"
#include "iscriptinterface.h"

namespace script
{

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

	void setMin(float min, int inMetres)
	{
		_radii.setMin(min, static_cast<bool>(inMetres));
	}

	void setMax(float max, int inMetres)
	{
		_radii.setMax(max, static_cast<bool>(inMetres));
	}

	float getMin(int inMetres) const
	{
		return _radii.getMin(static_cast<bool>(inMetres));
	}

	float getMax(int inMetres) const
	{
		return _radii.getMax(static_cast<bool>(inMetres));
	}
};

/**
 * This class represents a single sound shader for a script.
 */
class ScriptSoundShader
{
	ISoundShader::Ptr _shader;

public:
	ScriptSoundShader(const ISoundShader::Ptr& shader) :
		_shader(shader)
	{}

	operator ISoundShader::Ptr&() {
		return _shader;
	}

	std::string getName()
	{
		return _shader ? _shader->getDeclName() : "";
	}

	ScriptSoundRadii getRadii()
	{
		return _shader ? ScriptSoundRadii(_shader->getRadii()) : ScriptSoundRadii();
	}

	SoundFileList getSoundFileList()
	{
		return _shader ? _shader->getSoundFileList() : SoundFileList();
	}

	std::string getShaderFilePath()
	{
		return _shader ? _shader->getDeclFilePath() : std::string();
	}

	std::string getDefinition()
	{
		return _shader ? _shader->getBlockSyntax().contents : std::string();
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
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
