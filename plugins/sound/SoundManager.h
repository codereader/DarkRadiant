#pragma once

#include "SoundShader.h"
#include "SoundPlayer.h"

#include "isound.h"
#include "icommandsystem.h"

#include "parser/ThreadedDeclParser.h"
#include "SoundFileLoader.h"
#include <map>

namespace sound
{

/// SoundManager implementing class.
class SoundManager final :
    public ISoundManager
{
private:
    // Master map of shaders
	ShaderMap _shaders;

    // Shaders are loaded asynchronically, this loader
    // takes care of the worker thread
    SoundFileLoader _defLoader;

	SoundShader::Ptr _emptyShader;

	// The helper class for playing the sounds
	std::unique_ptr<SoundPlayer> _soundPlayer;

    sigc::signal<void> _sigSoundShadersReloaded;

private:
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
    float getSoundFileDuration(const std::string& vfsPath) override;
    sigc::signal<void>& signal_soundShadersReloaded() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
};

}
