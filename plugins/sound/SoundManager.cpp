#include "SoundManager.h"
#include "SoundFileLoader.h"

#include "ifilesystem.h"
#include "archivelib.h"
#include "generic/callback.h"
#include "parser/DefBlockTokeniser.h"

#include "debugging/ScopedDebugTimer.h"

#include <iostream>

namespace sound
{

// Constructor
SoundManager::SoundManager() :
	_emptyShader(new SoundShader("", ""))
{}

// Enumerate shaders
void SoundManager::forEachShader(SoundShaderVisitor& visitor) const {
	for (ShaderMap::const_iterator i = _shaders.begin();
		 i != _shaders.end();
		 ++i)
	{
		visitor.visit(i->second);
	}
}

bool SoundManager::playSound(const std::string& fileName) {
	// Make a copy of the filename
	std::string name = fileName; 
	
	// Try to open the file as it is
	ArchiveFilePtr file = GlobalFileSystem().openFile(name);
	std::cout << "Trying: " << name << std::endl;
	if (file != NULL) {
		// File found, play it
		std::cout << "Found file: " << name << std::endl;
		_soundPlayer.play(*file);
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
		_soundPlayer.play(*file);
		return true;
	}
	
	// Try to open the file with .wav extension
	name = root + ".wav";
	std::cout << "Trying: " << name << std::endl;
	file = GlobalFileSystem().openFile(name);
	if (file != NULL) {
		std::cout << "Found file: " << name << std::endl;
		_soundPlayer.play(*file);
		return true;
	}
	
	// File not found
	return false;
}

void SoundManager::stopSound() {
	_soundPlayer.stop();
}

// Accept a string of shaders to parse
void SoundManager::parseShadersFrom(std::istream& contents, const std::string& modName)
{
	// Construct a DefTokeniser to tokenise the string into sound shader decls
	parser::BasicDefBlockTokeniser<std::istream> tok(contents);

	while (tok.hasMoreBlocks()) {
		// Retrieve a named definition block from the parser
		parser::BlockTokeniser::Block block = tok.nextBlock();

		// Create a new shader with this name
		std::pair<ShaderMap::iterator, bool> result = _shaders.insert(
			ShaderMap::value_type(
				block.name, 
				SoundShaderPtr(new SoundShader(block.name, block.contents, modName))
			)
		);

		if (!result.second) {
			globalErrorStream() << "[SoundManager]: SoundShader with name " 
				<< block.name << " already exists." << std::endl;
		}
	}
}

ISoundShaderPtr SoundManager::getSoundShader(const std::string& shaderName) {
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

void SoundManager::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "SoundManager::initialiseModule called\n";
	// Pass a SoundFileLoader to the filesystem
	SoundFileLoader loader(*this);

	{
		ScopedDebugTimer timer("Sound definitions parsed: ");
		GlobalFileSystem().forEachFile(
			SOUND_FOLDER,			// directory 
			"sndshd", 				// required extension
			makeCallback1(loader),	// loader callback
			99						// max depth
		);
	}

	globalOutputStream() << _shaders.size() << " sound shaders found." << std::endl;
}

} // namespace sound
