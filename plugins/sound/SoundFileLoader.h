#ifndef SOUNDFILELOADER_H_
#define SOUNDFILELOADER_H_

#include "SoundManager.h"

#include "parser/DefBlockTokeniser.h"
#include "parser/DefTokeniser.h"
#include "ifilesystem.h"
#include "iarchive.h"

#include <iostream>

namespace sound
{

/**
 * Sound directory name.
 */
const char* SOUND_FOLDER = "sound/";

/**
 * Loader class passed to the GlobalFileSystem to load sound files
 */
class SoundFileLoader :
	public VirtualFileSystem::Visitor
{
    // Shader map to populate
	SoundManager::ShaderMap& _shaders;
	
private:

    // Accept a stream of shaders to parse
    void parseShadersFromStream(std::istream& contents,
                                const std::string& modName)
    {
        // Construct a DefTokeniser to tokenise the string into sound shader
        // decls
        parser::BasicDefBlockTokeniser<std::istream> tok(contents);

        while (tok.hasMoreBlocks()) {
            // Retrieve a named definition block from the parser
            parser::BlockTokeniser::Block block = tok.nextBlock();

            // Create a new shader with this name
            std::pair<SoundManager::ShaderMap::iterator, bool> result;
            result = _shaders.insert(
                SoundManager::ShaderMap::value_type(
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

public:

	/**
	 * Constructor. Set the sound manager reference.
	 */
	SoundFileLoader(SoundManager::ShaderMap& shaderMap)
	: _shaders(shaderMap)
	{ }	

	/**
	 * Functor operator.
	 */
	void visit(const std::string& filename)
	{
		// Open the .sndshd file and get its contents as a std::string
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(SOUND_FOLDER + filename);
		
		// Parse contents of file if it was opened successfully
		if (file) 
        {
			std::istream is(&(file->getInputStream()));
	
			try 
            {
				parseShadersFromStream(is, file->getModName());
			}
			catch (parser::ParseException& ex) {
				globalErrorStream() << "[sound]: Error while parsing " << filename <<
					": " << ex.what() << std::endl;
			}
		}
		else {
			std::cerr << "[sound] Warning: unable to open \"" 
					  << filename << "\"" << std::endl;
		}
	}
};

}

#endif /*SOUNDFILELOADER_H_*/
