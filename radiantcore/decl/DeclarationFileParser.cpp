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
{
#if 0
    _defaultTypeCreator = getCreatorByType(declType);

    if (!_defaultTypeCreator)
    {
        throw std::invalid_argument("No creator has been associated to the default type " + getTypeName(declType));
    }
#endif
}

std::map<Type, std::vector<DeclarationBlockSyntax>>& DeclarationFileParser::getParsedBlocks()
{
    return _parsedBlocks;
}

const std::set<DeclarationFile>& DeclarationFileParser::getParsedFiles() const
{
    return _parsedFiles;
}

void DeclarationFileParser::parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir)
{
    _parsedFiles.emplace(DeclarationFile{ fileInfo.fullPath(), _defaultDeclType });

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
        auto spacePos = block.name.find(' ');

        if (spacePos == std::string::npos)
        {
            // No type specified, use the default type creator
            processBlock(*_defaultTypeCreator, blockSyntax);
            continue;
        }

        // Locate a creator capable of handling that block
        auto creator = _creatorsByTypename.find(block.name.substr(0, spacePos));

        if (creator != _creatorsByTypename.end())
        {
            processBlock(*creator->second, blockSyntax);
            continue;
        }

        // Unknown block type, move to buffer
        _unrecognisedBlocks.emplace_back(std::move(blockSyntax));
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


#if 0
void DeclarationFileParser::processBlock(IDeclarationCreator& creator, const DeclarationBlockSyntax& block)
{
    auto declaration = creator.createDeclaration(block.name);

    declaration->setBlockSyntax(block);

    auto& declMap = _parsedDecls.try_emplace(creator.getDeclType(), NamedDeclarations()).first->second;

    // Insert into map, emit a warning on duplicates
    DeclarationManager::InsertDeclaration(declMap, std::move(declaration));
}

IDeclarationCreator::Ptr DeclarationFileParser::getCreatorByType(Type declType) const
{
    // Get the default type creator
    for (const auto& pair : _creatorsByTypename)
    {
        if (pair.second->getDeclType() == declType)
        {
            return pair.second;
        }
    }

    return {};
}
#endif

}
