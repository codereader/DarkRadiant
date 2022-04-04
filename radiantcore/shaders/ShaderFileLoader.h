#pragma once

#include <regex>

#include "iarchive.h"
#include "ifilesystem.h"

#include "TableDefinition.h"
#include "ShaderTemplate.h"
#include "ShaderDefinition.h"
#include "ShaderLibrary.h"

#include "parser/DefBlockTokeniser.h"
#include "parser/ThreadedDeclParser.h"
#include "materials/ParseLib.h"
#include "string/replace.h"
#include "string/predicate.h"
#include "debugging/ScopedDebugTimer.h"

namespace shaders
{

// VFS functor class which loads material (mtr) files.
class ShaderFileLoader : 
    public parser::ThreadedDeclParser<ShaderLibraryPtr>
{
private:
    ShaderLibraryPtr _library;

public:
    /// Construct and initialise the ShaderFileLoader
    ShaderFileLoader() :
        parser::ThreadedDeclParser<ShaderLibraryPtr>(decl::Type::Material, 
            getMaterialsFolderName(), getMaterialFileExtension(), 1)
    {}

protected:
    void onBeginParsing() override
    {
        // Load the shader files from the VFS into a fresh library
        _library = std::make_shared<ShaderLibrary>();
    }

    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override
    {
        // Parse the file with a blocktokeniser, the actual block contents
        // will be parsed separately.
        parser::BasicDefBlockTokeniser<std::istream> tokeniser(stream);

        while (tokeniser.hasMoreBlocks())
        {
            // Get the next block
            parser::BlockTokeniser::Block block = tokeniser.nextBlock();

            // Try to parse tables
            if (parseTable(block, fileInfo))
            {
                continue; // table successfully parsed
            }

            if (block.name.substr(0, 5) == "skin ")
            {
                continue; // skip skin definition
            }

            if (block.name.substr(0, 9) == "particle ")
            {
                continue; // skip particle definition
            }

            string::replace_all(block.name, "\\", "/"); // use forward slashes

            auto shaderTemplate = std::make_shared<ShaderTemplate>(block.name, block.contents);

            // Construct the ShaderDefinition wrapper class
            ShaderDefinition def(shaderTemplate, fileInfo);

            // Insert into the definitions map, if not already present
            if (!_library->addDefinition(block.name, def))
            {
                rError() << "[shaders] " << fileInfo.name << ": shader " << block.name << " already defined." << std::endl;
            }
        }
    }

    ShaderLibraryPtr onFinishParsing() override
    {
        rMessage() << _library->getNumDefinitions() << " shader definitions found." << std::endl;

        // Move the resource contained in the local shared_ptr
        return std::move(_library);
    }

private:
    bool parseTable(const parser::BlockTokeniser::Block& block, const vfs::FileInfo& fileInfo)
    {
        if (block.name.length() <= 5 || !string::starts_with(block.name, "table"))
        {
            return false; // definitely not a table decl
        }

        // Look closer by trying to split up the table name from the decl
        // it can still be a material starting with "table_" (#5188)
        std::regex expr("^table\\s+(.+)$");
        std::smatch matches;

        if (std::regex_match(block.name, matches, expr))
        {
            auto tableName = matches[1].str();

            auto table = std::make_shared<TableDefinition>(tableName, block.contents);

            if (!_library->addTableDefinition(table))
            {
                rError() << "[shaders] " << fileInfo.name << ": table " << tableName << " already defined." << std::endl;
            }

            return true;
        }

        return false;
    }
};

}
