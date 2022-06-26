#include "DeclarationFileParser.h"

#include "DeclarationManager.h"
#include "parser/DefBlockTokeniser.h"
#include "string/trim.h"

namespace decl
{

namespace
{
    inline std::string getBlockTypeName(const std::string& declaration)
    {
        auto spacePos = declaration.find(' ');

        if (spacePos == std::string::npos) return {};

        return string::trim_copy(declaration.substr(0, spacePos), " \t"); // remove all tabs and spaces
    }

    inline DeclarationBlockSyntax createBlock(const parser::BlockTokeniser::Block& block,
        const vfs::FileInfo& fileInfo, const std::string& modName)
    {
        auto spacePos = block.name.find(' ');

        DeclarationBlockSyntax syntax;

        syntax.typeName = getBlockTypeName(block.name);
        syntax.name = spacePos != std::string::npos ? block.name.substr(spacePos + 1) : block.name;
        syntax.contents = block.contents;
        syntax.modName = modName;
        syntax.fileInfo = fileInfo;

        return syntax;
    }
}

DeclarationFileParser::DeclarationFileParser(Type defaultDeclType, const std::map<std::string, Type>& typeMapping) :
    _defaultDeclType(defaultDeclType),
    _typeMapping(typeMapping)
{}

std::map<Type, std::vector<DeclarationBlockSyntax>>& DeclarationFileParser::getParsedBlocks()
{
    return _parsedBlocks;
}

void DeclarationFileParser::parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir)
{
#if 0
    auto& declFile = _parsedFiles.emplace_back(DeclarationFile{ fileInfo.fullPath(), _defaultDeclType });
#endif
    // Cut the incoming stream into declaration blocks
    parser::BasicDefBlockTokeniser<std::istream> tokeniser(stream);

    while (tokeniser.hasMoreBlocks())
    {
        auto block = tokeniser.nextBlock();

        // Convert the incoming block to a DeclarationBlockSyntax
        auto blockSyntax = createBlock(block, fileInfo, modDir);

        // Move the block in the correct bucket
        auto declType = determineBlockType(blockSyntax);
        auto& blockList = _parsedBlocks.try_emplace(declType).first->second;
        blockList.emplace_back(std::move(blockSyntax));
#if 0
        // Add this declaration to the parsed file
        //declFile.declarations.emplace_back(declType, block.name);
#endif
    }
}

Type DeclarationFileParser::determineBlockType(const DeclarationBlockSyntax& block)
{
    if (block.typeName.empty())
    {
        return _defaultDeclType;
    }

    auto foundType = _typeMapping.find(block.typeName);

    return foundType != _typeMapping.end() ? foundType->second : Type::Undetermined;
}

}
