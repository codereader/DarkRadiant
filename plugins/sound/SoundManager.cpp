#include "SoundManager.h"
#include "SoundFileLoader.h"

#include "ifilesystem.h"
#include "archivelib.h"

#include "debugging/ScopedDebugTimer.h"

#include <iostream>

namespace sound
{

// Constructor
SoundManager::SoundManager() :
	_emptyShader(new SoundShader("", "")),
    _shadersLoaded(false)
{}

// Enumerate shaders
void SoundManager::forEachShader(boost::function<void(const ISoundShader&)> f)
const
{
    ensureShadersLoaded();

	for (ShaderMap::const_iterator i = _shaders.begin();
		 i != _shaders.end();
		 ++i)
	{
		f(*i->second);
	}
}

bool SoundManager::playSound(const std::string& fileName)
{
	// Make a copy of the filename
	std::string name = fileName;

	// Try to open the file as it is
	ArchiveFilePtr file = GlobalFileSystem().openFile(name);
	std::cout << "Trying: " << name << std::endl;
	if (file != NULL) {
		// File found, play it
		std::cout << "Found file: " << name << std::endl;
		if (_soundPlayer) _soundPlayer->play(*file);
		return true;
	}

	std::string root = name;
	// File not found, try to strip the extension
	if (name.rfind(".") != std::string::npos) {
		root = name.substr(0, name.rfind("."));
	}

	// Try to open the .ogg variant
	name = root + ".ogg";
	std::cout << "Trying: " << name << std::endl;
	file = GlobalFileSystem().openFile(name);
	if (file != NULL) {
		std::cout << "Found file: " << name << std::endl;
		if (_soundPlayer) _soundPlayer->play(*file);
		return true;
	}

	// Try to open the file with .wav extension
	name = root + ".wav";
	std::cout << "Trying: " << name << std::endl;
	file = GlobalFileSystem().openFile(name);
	if (file != NULL) {
		std::cout << "Found file: " << name << std::endl;
		if (_soundPlayer) _soundPlayer->play(*file);
		return true;
	}

	// File not found
	return false;
}

void SoundManager::stopSound() {
	if (_soundPlayer) _soundPlayer->stop();
}

ISoundShaderPtr SoundManager::getSoundShader(const std::string& shaderName)
{
    ensureShadersLoaded();
	ShaderMap::const_iterator found = _shaders.find(shaderName);

    // If the name was found, return it, otherwise return an empty shader object
	return (found != _shaders.end()) ? found->second : _emptyShader;
}

const std::string& SoundManager::getName() const {
	static std::string _name(MODULE_SOUNDMANAGER);
	return _name;
}

const StringSet& SoundManager::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void SoundManager::loadShadersFromFilesystem() const
{
	// Pass a SoundFileLoader to the filesystem
	SoundFileLoader loader(_shaders);

    GlobalFileSystem().forEachFile(
        SOUND_FOLDER,			// directory
        "sndshd", 				// required extension
        loader,	// loader callback
        99						// max depth
    );

    rMessage() << _shaders.size()
                            << " sound shaders found." << std::endl;

    _shadersLoaded = true;
}

void SoundManager::ensureShadersLoaded() const
{
    if (!_shadersLoaded)
    {
        loadShadersFromFilesystem();
    }
}

void SoundManager::initialiseModule(const ApplicationContext& ctx)
{
    // Create the SoundPlayer if sound is not disabled
    const ApplicationContext::ArgumentList& args = ctx.getCmdLineArgs();
    ApplicationContext::ArgumentList::const_iterator found(
        std::find(args.begin(), args.end(), "--disable-sound")
    );
    if (found == args.end())
    {
        rMessage() << "SoundManager: initialising sound playback"
                             << std::endl;
        _soundPlayer = boost::shared_ptr<SoundPlayer>(new SoundPlayer);
    }
    else
    {
        rMessage() << "SoundManager: sound ouput disabled"
                             << std::endl;
    }
}

} // namespace sound
