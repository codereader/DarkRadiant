#include "SoundManager.h"
#include "SoundFileLoader.h"

#include "ifilesystem.h"
#include "generic/callback.h"
#include "parser/DefTokeniser.h"

#include <iostream>

namespace sound
{

// Constructor
SoundManager::SoundManager()
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

// Accept a string of shaders to parse
void SoundManager::parseShadersFrom(const std::string& contents) {
	
	// Construct a DefTokeniser to tokeniser the string into sound shader decls
	parser::DefTokeniser tok(contents);
	while (tok.hasMoreTokens())
		parseSoundShader(tok);
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
