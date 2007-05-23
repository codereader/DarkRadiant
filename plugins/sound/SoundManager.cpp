#include "SoundManager.h"
#include "SoundFileLoader.h"

#include "ifilesystem.h"
#include "generic/callback.h"
#include "parser/DefTokeniser.h"

#include <iostream>

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
	
	// Construct a DefTokeniser to tokeniser the string into sound shader decls
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
	tok.assertNextToken("{");
	
	while (tok.nextToken() != "}");	
	
	// Add the shader to the map
	_shaders.insert(
		ShaderMap::value_type(
			name, 
			ShaderPtr(new SoundShader(name))
		)
	);
}

}
