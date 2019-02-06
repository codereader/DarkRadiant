#include "SoundManager.h"
#include "SoundFileLoader.h"

#include "ifilesystem.h"

#include "debugging/ScopedDebugTimer.h"

#include <algorithm>
#include "itextstream.h"

namespace sound
{

// Constructor
SoundManager::SoundManager() :
    _defLoader(std::bind(&SoundManager::loadShadersFromFilesystem, this)),
	_emptyShader(new SoundShader("", ""))
{}

// Enumerate shaders
void SoundManager::forEachShader(std::function<void(const ISoundShader&)> f)
{
    ensureShadersLoaded();

	for (const ShaderMap::value_type& pair : _shaders)
	{
		f(*pair.second);
	}
}

bool SoundManager::playSound(const std::string& fileName)
{
	return playSound(fileName, false);
}

bool SoundManager::playSound(const std::string& fileName, bool loopSound)
{
	// Make a copy of the filename
	std::string name = fileName;

	// Try to open the file as it is
	ArchiveFilePtr file = GlobalFileSystem().openFile(name);
    rConsole() << "Trying: " << name << std::endl;

	if (file) 
	{
		// File found, play it
        rConsole() << "Found file: " << name << std::endl;
		if (_soundPlayer) _soundPlayer->play(*file, loopSound);
		return true;
	}

	std::string root = name;

	// File not found, try to strip the extension
	if (name.rfind(".") != std::string::npos)
	{
		root = name.substr(0, name.rfind("."));
	}

	// Try to open the .ogg variant
	name = root + ".ogg";

    rConsole() << "Trying: " << name << std::endl;

	file = GlobalFileSystem().openFile(name);

	if (file) 
	{
        rConsole() << "Found file: " << name << std::endl;
		if (_soundPlayer) _soundPlayer->play(*file, loopSound);
		return true;
	}

	// Try to open the file with .wav extension
	name = root + ".wav";
    rConsole() << "Trying: " << name << std::endl;

	file = GlobalFileSystem().openFile(name);

	if (file)
	{
        rConsole() << "Found file: " << name << std::endl;
		if (_soundPlayer) _soundPlayer->play(*file, loopSound);
		return true;
	}

	// File not found
	return false;
}

void SoundManager::stopSound()
{
	if (_soundPlayer) _soundPlayer->stop();
}

ISoundShaderPtr SoundManager::getSoundShader(const std::string& shaderName)
{
    ensureShadersLoaded();

	ShaderMap::const_iterator found = _shaders.find(shaderName);

    // If the name was found, return it, otherwise return an empty shader object
	return found != _shaders.end() ? found->second : _emptyShader;
}

const std::string& SoundManager::getName() const
{
	static std::string _name(MODULE_SOUNDMANAGER);
	return _name;
}

const StringSet& SoundManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void SoundManager::loadShadersFromFilesystem()
{
    ShaderMapPtr foundShaders = std::make_shared<ShaderMap>();

	// Pass a SoundFileLoader to the filesystem
    SoundFileLoader loader(*foundShaders);

    GlobalFileSystem().forEachFile(
        SOUND_FOLDER,			// directory
        "sndshd", 				// required extension
        [&](const std::string& filename, vfs::Visibility) { loader(filename); },	// loader callback
        99						// max depth
    );

    _shaders.swap(*foundShaders);

    rMessage() << _shaders.size() << " sound shaders found." << std::endl;
}

void SoundManager::ensureShadersLoaded()
{
    _defLoader.ensureFinished();
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
        _soundPlayer = std::shared_ptr<SoundPlayer>(new SoundPlayer);
    }
    else
    {
        rMessage() << "SoundManager: sound output disabled"
                             << std::endl;
    }

    _defLoader.start();
}

} // namespace sound
