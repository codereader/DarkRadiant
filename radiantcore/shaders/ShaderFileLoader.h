#pragma once

#include <regex>

#include "iarchive.h"
#include "ifilesystem.h"

#include "TableDefinition.h"
#include "ShaderTemplate.h"
#include "ShaderDefinition.h"

#include "parser/DefBlockTokeniser.h"
#include "string/replace.h"
#include "string/trim.h"
#include "string/predicate.h"
#include "os/path.h"

#include "parser/Grammar.h"

#include <tao/pegtl/contrib/parse_tree.hpp>

namespace shaders
{

// VFS functor class which loads material (mtr) files.
template<typename ShaderLibrary_T> class ShaderFileLoader
{
    // The VFS module to provide shader files
    vfs::VirtualFileSystem& _vfs;

    ShaderLibrary_T& _library;

    // List of shader definition files to parse
    std::vector<vfs::FileInfo> _files;

    class BlockParseState
    {
    public:
        BlockParseState(ShaderLibrary_T& library_, const vfs::FileInfo& fileInfo_) :
            fileInfo(fileInfo_),
            library(library_)
        {}

        const vfs::FileInfo& fileInfo;
        ShaderLibrary_T& library;

        std::string name;
        std::string contents;
    };

    template<typename Rule>
    struct ParseAction : tao::pegtl::nothing<Rule> {};

    template<>
    struct ParseAction<parser::DeclName>
    {
        template<typename ParseInput>
        static void apply(const ParseInput& in, BlockParseState& state)
        {
            state.name = in.string();
            state.contents.clear();
        }
    };

    template<>
    struct ParseAction<parser::MaterialBlockDefinition>
    {
        template<typename ParseInput>
        static void apply(const ParseInput& in, BlockParseState& state)
        {
            state.contents = string::trim_right_copy(string::trim_left_copy(in.string(), "{"), "}");
                        
            string::replace_all(state.name, "\\", "/"); // use forward slashes

            auto shaderTemplate = std::make_shared<ShaderTemplate>(state.name, state.contents);

            // Construct the ShaderDefinition wrapper class
            ShaderDefinition def(shaderTemplate, state.fileInfo);

            // Insert into the definitions map, if not already present
            if (!state.library.addDefinition(state.name, def))
            {
                rError() << "[shaders] " << state.fileInfo.name << ": shader " << state.name << " already defined." << std::endl;
            }
        }
    };

private:

    bool parseTable(const ::parser::BlockTokeniser::Block& block, const vfs::FileInfo& fileInfo)
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

            if (!_library.addTableDefinition(table))
            {
                rError() << "[shaders] " << fileInfo.name << ": table " << tableName << " already defined." << std::endl;
            }

            return true;
        }

        return false;
    }

    // Parse a shader file with the given contents and filename
    void parseShaderFile(std::istream& inStr, const vfs::FileInfo& fileInfo)
    {
#if 1
        using namespace tao::pegtl;

        // Wrap the input stream for the pegtl parser
        istream_input input(inStr, 1*1024*1024, os::getFilename(fileInfo.name));

        BlockParseState state(_library, fileInfo);

        try
        {
            auto parseTree = parse_tree::parse<parser::MaterialFile, parser::MaterialFileSelector, ParseAction>(input, state);
        }
        catch (const parser::parse_error& ex)
        {
            rError() << "Error parsing " << fileInfo.name << ": " << ex.what() << std::endl;
        }
#else
        // Parse the file with a blocktokeniser, the actual block contents
        // will be parsed separately.
        ::parser::BasicDefBlockTokeniser<std::istream> tokeniser(inStr);

        while (tokeniser.hasMoreBlocks())
        {
            // Get the next block
            ::parser::BlockTokeniser::Block block = tokeniser.nextBlock();

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
            if (!_library.addDefinition(block.name, def))
            {
                rError() << "[shaders] " << fileInfo.name << ": shader " << block.name << " already defined." << std::endl;
            }
        }
#endif
    }

public:

    /// Construct and initialise the ShaderFileLoader
    ShaderFileLoader(vfs::VirtualFileSystem& fs, ShaderLibrary_T& library,
                     const std::string& basedir,
                     const std::string& extension = "mtr")
    : _vfs(fs), _library(library)
    {
        _files.reserve(200);

        // Walk the VFS and populate our files list
        _vfs.forEachFile(
            basedir, extension,
            [this](const vfs::FileInfo& fi) { _files.push_back(fi); },
            0
        );
    }

    void parseFiles()
    {
        for (const vfs::FileInfo& fileInfo: _files)
        {
            // Open the file
            auto file = _vfs.openTextFile(fileInfo.fullPath());

            if (file)
            {
                std::istream is(&(file->getInputStream()));
                parseShaderFile(is, fileInfo);
            }
            else
            {
                throw std::runtime_error("Unable to read shaderfile: " + fileInfo.name);
            }
        }
    }
};

}
