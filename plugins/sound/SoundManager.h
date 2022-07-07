#pragma once

#include "SoundShader.h"
#include "SoundPlayer.h"

#include "isound.h"
#include "icommandsystem.h"

namespace sound
{

/// SoundManager implementing class.
class SoundManager final :
    public ISoundManager
{
private:
	SoundShader::Ptr _emptyShader;

	// The helper class for playing the sounds
	std::unique_ptr<SoundPlayer> _soundPlayer;

    sigc::signal<void> _sigSoundShadersReloaded;

public:
	SoundManager();

    // ISoundManager implementation
	void forEachShader(std::function<void(const ISoundShader::Ptr&)>) override;
	ISoundShader::Ptr getSoundShader(const std::string& shaderName) override;
	bool playSound(const std::string& fileName) override;
	bool playSound(const std::string& fileName, bool loopSound) override;
	void stopSound() override;
    void reloadSounds() override;
    float getSoundFileDuration(const std::string& vfsPath) override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
};

}
