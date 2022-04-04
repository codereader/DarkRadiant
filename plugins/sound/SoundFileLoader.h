#pragma once

#include <map>
#include "SoundManager.h"

#include "parser/DefBlockTokeniser.h"
#include "parser/DefTokeniser.h"
#include "parser/ThreadedDeclParser.h"
#include "ifilesystem.h"
#include "iarchive.h"
#include "ui/imainframe.h"

#include <iostream>

namespace sound
{

namespace
{
    /// Sound directory name
    constexpr const char* const SOUND_FOLDER = "sound/";
    constexpr const char* const SOUND_FILE_EXTENSION = "sndshd";
}

using ShaderMap = std::map<std::string, SoundShader::Ptr>;

/**
 * Threaded parser class loading the sndshd files
 */
class SoundFileLoader final :
    public parser::ThreadedDeclParser<ShaderMap>
{
private:
    // Shader map to populate
    ShaderMap _shaders;

public:
    SoundFileLoader() :
        parser::ThreadedDeclParser<ShaderMap>(
            decl::Type::SoundShader, SOUND_FOLDER, SOUND_FILE_EXTENSION, 99)
    {}

protected:
    void onBeginParsing() override
    {
        _shaders.clear();
    }

    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override
    {
        // Construct a DefTokeniser to tokenise the string into sound shader decls
        parser::BasicDefBlockTokeniser<std::istream> tok(stream);

        while (tok.hasMoreBlocks())
        {
            // Retrieve a named definition block from the parser
            parser::BlockTokeniser::Block block = tok.nextBlock();

            // Create a new shader with this name
            auto result = _shaders.emplace(block.name,
                std::make_shared<SoundShader>(block.name, block.contents, fileInfo, modDir)
            );

            if (!result.second)
            {
                rError() << "[SoundManager]: SoundShader with name "
                    << block.name << " already exists." << std::endl;
            }
        }
    }

    ShaderMap onFinishParsing() override
    {
        rMessage() << _shaders.size() << " sound shaders found." << std::endl;

        return std::move(_shaders);
    }
};

}
