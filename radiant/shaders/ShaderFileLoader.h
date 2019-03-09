#pragma once

#include "iarchive.h"

#include "TableDefinition.h"
#include "ShaderTemplate.h"
#include "ShaderDefinition.h"

#include "parser/DefBlockTokeniser.h"
#include "string/replace.h"

namespace shaders
{

// VFS functor class which loads material (mtr) files.
template<typename ShaderLibrary_T> class ShaderFileLoader
{
    // The base path for the shaders (e.g. "materials/")
    std::string _basePath;

    ShaderLibrary_T& _library;

    // List of shader definition files to parse
    std::vector<vfs::FileInfo> _files;

private:

    // Parse a shader file with the given contents and filename
    void parseShaderFile(std::istream& inStr, const vfs::FileInfo& fileInfo)
    {
        // Parse the file with a blocktokeniser, the actual block contents
        // will be parsed separately.
        parser::BasicDefBlockTokeniser<std::istream> tokeniser(inStr);

        while (tokeniser.hasMoreBlocks())
        {
            // Get the next block
            parser::BlockTokeniser::Block block = tokeniser.nextBlock();

            // Skip tables
            if (block.name.substr(0, 5) == "table")
            {
                std::string tableName = block.name.substr(6);

                if (tableName.empty())
                {
                    rError() << "[shaders] " << fileInfo.name << ": Missing table name." << std::endl;
                    continue;
                }

                TableDefinitionPtr table(new TableDefinition(tableName, block.contents));

                if (!_library.addTableDefinition(table))
                {
                    rError() << "[shaders] " << fileInfo.name
                        << ": table " << tableName << " already defined." << std::endl;
                }

                continue;
            }
            else if (block.name.substr(0, 5) == "skin ")
            {
                continue; // skip skin definition
            }
            else if (block.name.substr(0, 9) == "particle ")
            {
                continue; // skip particle definition
            }

            string::replace_all(block.name, "\\", "/"); // use forward slashes

            ShaderTemplatePtr shaderTemplate(new ShaderTemplate(block.name, block.contents));

            // Construct the ShaderDefinition wrapper class
            ShaderDefinition def(shaderTemplate, fileInfo.name);

            // Insert into the definitions map, if not already present
            if (!_library.addDefinition(block.name, def))
            {
                rError() << "[shaders] " << fileInfo.name
                    << ": shader " << block.name << " already defined." << std::endl;
            }
        }
    }

public:
    // Constructor. Set the basepath to prepend onto shader filenames.
    ShaderFileLoader(const std::string& path, ShaderLibrary_T& library)
    : _basePath(path), _library(library)
    {
        _files.reserve(200);
    }

    void addFile(const vfs::FileInfo& fileInfo)
    {
        // Construct the full VFS path
        vfs::FileInfo fileWithBasePath = fileInfo;
        fileWithBasePath.name = _basePath + fileInfo.name;
        _files.push_back(fileWithBasePath);
    }

    void parseFiles()
    {
        for (std::size_t i = 0; i < _files.size(); ++i)
        {
            const vfs::FileInfo& fileInfo = _files[i];

            // Open the file
            ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(fileInfo.name);

            if (file != nullptr)
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
