#include "SoundManager.h"
#include "SoundFileLoader.h"

#include "ifilesystem.h"
#include "archivelib.h"

#include "debugging/ScopedDebugTimer.h"

#include "itextstream.h"

namespace sound
{

// Constructor
SoundManager::SoundManager() :
	_emptyShader(new SoundShader("", "")),
    _shadersLoaded(false)
{}

// Enumerate shaders
void SoundManager::forEachShader(std::function<void(const ISoundShader&)> f)
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
    rConsole() << "Trying: " << name << std::endl;
	if (file != NULL) {
		// File found, play it
        rConsole() << "Found file: " << name << std::endl;
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
    rConsole() << "Trying: " << name << std::endl;
	file = GlobalFileSystem().openFile(name);
	if (file != NULL) {
        rConsole() << "Found file: " << name << std::endl;
		if (_soundPlayer) _soundPlayer->play(*file);
		return true;
	}

	// Try to open the file with .wav extension
	name = root + ".wav";
    rConsole() << "Trying: " << name << std::endl;
	file = GlobalFileSystem().openFile(name);
	if (file != NULL) {
        rConsole() << "Found file: " << name << std::endl;
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

SoundManager::ShaderMapPtr SoundManager::loadShadersFromFilesystem()
{
    ShaderMapPtr foundShaders = std::make_shared<ShaderMap>();

	// Pass a SoundFileLoader to the filesystem
    SoundFileLoader loader(*foundShaders);

    GlobalFileSystem().forEachFile(
        SOUND_FOLDER,			// directory
        "sndshd", 				// required extension
        [&](const std::string& filename) { loader(filename); },	// loader callback
        99						// max depth
    );

    return foundShaders;
}

void SoundManager::ensureShadersLoaded()
{
    if (!_shadersLoaded && !_foundShaders.valid())
    {
        // No shaders loaded and no one currently looking for them

        // Launch a new thread
        _foundShaders = std::async(std::launch::async,
            std::bind(&SoundManager::loadShadersFromFilesystem, this));
    }

    // If the thread is still running, block until it's done
    if (_foundShaders.valid())
    {
        _shaders.swap(*_foundShaders.get());
        _foundShaders = std::future<ShaderMapPtr>();
        rMessage() << _shaders.size() << " sound shaders found." << std::endl;
        _shadersLoaded = true;
    }

    // When reaching this point, the shaders should be loaded
    assert(_shadersLoaded);
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
        rMessage() << "SoundManager: sound ouput disabled"
                             << std::endl;
    }

    _foundShaders = std::async(std::launch::async,
        std::bind(&SoundManager::loadShadersFromFilesystem, this));
}

} // namespace sound
