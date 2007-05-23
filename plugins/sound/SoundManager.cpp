#include "SoundManager.h"
#include "SoundFileLoader.h"

#include "ifilesystem.h"
#include "generic/callback.h"
#include "parser/DefTokeniser.h"

#include <iostream>
#include <boost/algorithm/string/predicate.hpp>

namespace sound
{

// Constructor
SoundManager::SoundManager() :
	_emptyShader("")
{
	// Pass a SoundFileLoader to the filesystem
	SoundFileLoader loader(*this);
	GlobalFileSystem().forEachFile(
		SOUND_FOLDER,			// directory 
		"sndshd", 				// required extension
		makeCallback1(loader),	// loader callback
		99						// max depth
	);  
}

// Enumerate shaders
void SoundManager::forEachShader(SoundShaderVisitor visitor) const {
	for (ShaderMap::const_iterator i = _shaders.begin();
		 i != _shaders.end();
		 ++i)
	{
		visitor(*(i->second));	
	}
}

void SoundManager::playSoundShader(const ISoundShader& soundShader) {
	// Sound Shader resolving code goes here
}

// Accept a string of shaders to parse
void SoundManager::parseShadersFrom(const std::string& contents) {
	
	// Construct a DefTokeniser to tokenise the string into sound shader decls
	parser::DefTokeniser tok(contents);
	while (tok.hasMoreTokens())
		parseSoundShader(tok);
}

const ISoundShader& SoundManager::getSoundShader(const std::string& shaderName) {
	ShaderMap::const_iterator found = _shaders.find(shaderName);
	
	// If the name was found, return it, otherwise return an empty shader object
	return (found != _shaders.end()) ? *found->second : _emptyShader;    
}

// Parse a single sound shader from a token stream
void SoundManager::parseSoundShader(parser::DefTokeniser& tok) {
	
	// Get the shader name
	std::string name = tok.nextToken();
	
	// Create a new shader with this name
	_shaders[name] = ShaderPtr(new SoundShader(name));
	
	// A definition block must start here
	tok.assertNextToken("{");
	
	std::string nextToken = tok.nextToken();
	while (nextToken != "}") {
		// Watch out for sound file definitions
		if (boost::algorithm::starts_with(nextToken, "sound/")) {
			// Add this to the list
			_shaders[name]->addSoundFile(nextToken);
		}
		
		nextToken = tok.nextToken();
	}
}

}
