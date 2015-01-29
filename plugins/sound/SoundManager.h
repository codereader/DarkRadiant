#pragma once

#include "SoundShader.h"
#include "SoundPlayer.h"

#include "isound.h"

#include <future>
#include <map>

namespace sound {

/// SoundManager implementing class.
class SoundManager : public ISoundManager
{
public: /* TYPES */

	// Map of named sound shaders
	typedef std::map<std::string, SoundShaderPtr> ShaderMap;
    typedef std::shared_ptr<ShaderMap> ShaderMapPtr;

private: /* FIELDS */

    // Master map of shaders
	ShaderMap _shaders;

    // Shaders are loaded asynchronically, this future
    // will hold the found shaders once loading is complete
    std::future<ShaderMapPtr> _foundShaders;

	SoundShaderPtr _emptyShader;

	// The helper class for playing the sounds
	std::shared_ptr<SoundPlayer> _soundPlayer;

    bool _shadersLoaded;

private:
    ShaderMapPtr loadShadersFromFilesystem();
    void ensureShadersLoaded();

public:
	/**
	 * Main constructor.
	 */
	SoundManager();

    // ISoundManager implementation
	void forEachShader(std::function<void(const ISoundShader&)>);
	ISoundShaderPtr getSoundShader(const std::string& shaderName);
	virtual bool playSound(const std::string& fileName);
	virtual void stopSound();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef std::shared_ptr<SoundManager> SoundManagerPtr;

}
