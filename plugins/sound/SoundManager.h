#pragma once

#include "SoundShader.h"
#include "SoundPlayer.h"

#include "isound.h"

#include "ThreadedDefLoader.h"
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

    // Shaders are loaded asynchronically, this loader
    // takes care of the worker thread
    util::ThreadedDefLoader<void> _defLoader;

	SoundShaderPtr _emptyShader;

	// The helper class for playing the sounds
	std::shared_ptr<SoundPlayer> _soundPlayer;

private:
    void loadShadersFromFilesystem();
    void ensureShadersLoaded();

public:
	SoundManager();

    // ISoundManager implementation
	void forEachShader(std::function<void(const ISoundShader&)>) override;
	ISoundShaderPtr getSoundShader(const std::string& shaderName) override;
	virtual bool playSound(const std::string& fileName) override;
	virtual void stopSound() override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
};
typedef std::shared_ptr<SoundManager> SoundManagerPtr;

}
