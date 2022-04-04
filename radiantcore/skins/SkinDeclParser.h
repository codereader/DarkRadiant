#pragma once

#include "itextstream.h"
#include "parser/ThreadedDeclParser.h"
#include "parser/DefTokeniser.h"

namespace skins
{

namespace
{
    // CONSTANTS
    constexpr const char* const SKINS_FOLDER = "skins/";
    constexpr const char* const SKIN_FILE_EXTENSION = "skin";
}

struct SkinParseResult
{
    std::map<std::string, ModelSkinPtr> namedSkins;
    std::vector<std::string> allSkins;
    std::map<std::string, std::vector<std::string>> modelSkins;

    using Ptr = std::shared_ptr<SkinParseResult>;
};

class SkinDeclParser :
    public parser::ThreadedDeclParser<SkinParseResult::Ptr>
{
private:
    SkinParseResult::Ptr _result;

public:
    SkinDeclParser() :
        parser::ThreadedDeclParser<SkinParseResult::Ptr>(decl::Type::Skin, SKINS_FOLDER, SKIN_FILE_EXTENSION, 1)
    {}

protected:
    void onBeginParsing() override
    {
        _result = std::make_shared<SkinParseResult>();
    }
    
    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override
    {
        // Construct a DefTokeniser to parse the file
        parser::BasicDefTokeniser<std::istream> tokeniser(stream);

        // Call the parseSkin() function for each skin decl
        while (tokeniser.hasMoreTokens())
        {
            try
            {
                // Try to parse the skin
                Doom3ModelSkinPtr modelSkin = parseSkin(tokeniser);
                std::string skinName = modelSkin->getName();

                modelSkin->setSkinFileName(fileInfo.name);

                auto found = _result->namedSkins.find(skinName);

                // Is this already defined?
                if (found != _result->namedSkins.end())
                {
                    rWarning() << "[skins] in " << fileInfo.name << ": skin " + skinName +
                        " previously defined in " + found->second->getSkinFileName() + "!" << std::endl;
                    // Don't insert the skin into the list
                }
                else
                {
                    // Add the populated Doom3ModelSkin to the hashtable and the name to the
                    // list of all skins
                    _result->namedSkins.emplace(skinName, modelSkin);
                    _result->allSkins.emplace_back(skinName);
                }
            }
            catch (parser::ParseException& e)
            {
                rWarning() << "[skins]: in " << fileInfo.name << ": " << e.what() << std::endl;
            }
        }
    }

    SkinParseResult::Ptr onFinishParsing() override
    {
        rMessage() << "[skins] Found " << _result->allSkins.size() << " skins." << std::endl;

        return std::move(_result);
    }

private:
    // Parse an individual skin declaration
    Doom3ModelSkinPtr parseSkin(parser::DefTokeniser& tok)
    {
        // [ "skin" ] <name> "{"
        //			[ "model" <modelname> ]
        //			( <sourceTex> <destTex> )*
        // "}"

        // Parse the skin name, this is either the first token or the second token
        // (preceded by "skin")
        auto skinName = tok.nextToken();

        if (skinName == "skin")
        {
            skinName = tok.nextToken();
        }

        tok.assertNextToken("{");

        // Create the skin object
        auto skin = std::make_shared<Doom3ModelSkin>(skinName);

        // Read key/value pairs until end of decl
        auto key = tok.nextToken();

        while (key != "}")
        {
            // Read the value
            auto value = tok.nextToken();

            if (value == "}")
            {
                rWarning() << "[skins] Warning: '}' found where shader name expected in skin: "
                    << skinName << std::endl;
            }

            // If this is a model key, add to the model->skin map, otherwise assume
            // this is a remap declaration
            if (key == "model")
            {
                _result->modelSkins[value].push_back(skinName);
            }
            else
            {
                skin->addRemap(key, value);
            }

            // Get next key
            key = tok.nextToken();
        }

        return skin;
    }
};

}
