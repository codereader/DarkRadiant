#pragma once

#include "SoundManager.h"

#include "parser/DefBlockTokeniser.h"
#include "parser/DefTokeniser.h"
#include "ifilesystem.h"
#include "iarchive.h"
#include "ui/imainframe.h"

#include <iostream>

namespace sound
{

/// Sound directory name
const char* SOUND_FOLDER = "sound/";

/**
 * Loader class passed to the GlobalFileSystem to load sound files
 */
class SoundFileLoader
{
    // Shader map to populate
	SoundManager::ShaderMap& _shaders;

private:

	std::string getShortened(const std::string& input, std::size_t maxLength)
	{
		if (input.length() > maxLength)
		{
			std::size_t diff = input.length() - maxLength + 3; // 3 chars for the ellipsis
			std::size_t curLength = input.length();

			return input.substr(0, (curLength - diff) / 2) + "..." +
				input.substr((curLength + diff) / 2);
		}

		return input;
	}

    // Accept a stream of shaders to parse
    void parseShadersFromStream(std::istream& contents, const vfs::FileInfo& fileInfo,
                                const std::string& modName)
    {
        // Construct a DefTokeniser to tokenise the string into sound shader
        // decls
        parser::BasicDefBlockTokeniser<std::istream> tok(contents);

        while (tok.hasMoreBlocks())
        {
            // Retrieve a named definition block from the parser
            parser::BlockTokeniser::Block block = tok.nextBlock();

            // Create a new shader with this name
            auto result = _shaders.emplace(block.name,
				std::make_shared<SoundShader>(block.name, block.contents, fileInfo, modName)
            );

            if (!result.second) {
                rError() << "[SoundManager]: SoundShader with name "
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
	void parseShaderFile(const vfs::FileInfo& fileInfo)
	{
		// Open the .sndshd file and get its contents as a std::string
		auto file = GlobalFileSystem().openTextFile(SOUND_FOLDER + fileInfo.name);

		// Parse contents of file if it was opened successfully
		if (!file)
        {
			rWarning() << "[sound] Warning: unable to open \""
					  << fileInfo.name << "\"" << std::endl;
			return;
		}

		std::istream is(&(file->getInputStream()));

		try
		{
			parseShadersFromStream(is, fileInfo, file->getModName());
		}
		catch (parser::ParseException& ex)
		{
			rError() << "[sound]: Error while parsing " << fileInfo.name <<
				": " << ex.what() << std::endl;
		}
	}
};

}
