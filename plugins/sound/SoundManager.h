#pragma once

#include "SoundShader.h"
#include "SoundPlayer.h"

#include "isound.h"
#include "icommandsystem.h"

#include "ThreadedDefLoader.h"
#include <map>

namespace sound {

/// SoundManager implementing class.
class SoundManager : 
    public ISoundManager
{
public: /* TYPES */
	// Map of named sound shaders
	typedef std::map<std::string, SoundShader::Ptr> ShaderMap;

private: /* FIELDS */

    // Master map of shaders
	ShaderMap _shaders;

    // Shaders are loaded asynchronically, this loader
    // takes care of the worker thread
    util::ThreadedDefLoader<void> _defLoader;

	SoundShader::Ptr _emptyShader;

	// The helper class for playing the sounds
	std::unique_ptr<SoundPlayer> _soundPlayer;

    sigc::signal<void> _sigSoundShadersReloaded;

private:
    void loadShadersFromFilesystem();
    void ensureShadersLoaded();
    void reloadSoundsCmd(const cmd::ArgumentList& args);

public:
	SoundManager();

    // ISoundManager implementation
	void forEachShader(std::function<void(const ISoundShader&)>) override;
	ISoundShaderPtr getSoundShader(const std::string& shaderName) override;
	bool playSound(const std::string& fileName) override;
	bool playSound(const std::string& fileName, bool loopSound) override;
	void stopSound() override;
    void reloadSounds() override;
    sigc::signal<void>& signal_soundShadersReloaded() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
};

}
