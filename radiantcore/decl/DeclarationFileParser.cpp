#include "DeclarationFileParser.h"

#include "DeclarationManager.h"
#include "parser/DefBlockTokeniser.h"

namespace decl
{

namespace
{
    inline DeclarationBlockSyntax createBlock(const parser::BlockTokeniser::Block& block,
        const vfs::FileInfo& fileInfo, const std::string& modName)
    {
        auto spacePos = block.name.find(' ');

        DeclarationBlockSyntax syntax;

        syntax.typeName = spacePos != std::string::npos ? block.name.substr(0, spacePos) : std::string();
        syntax.name = spacePos != std::string::npos ? block.name.substr(spacePos + 1) : block.name;
        syntax.contents = block.contents;
        syntax.modName = modName;
        syntax.fileInfo = fileInfo;

        return syntax;
    }
}

DeclarationFileParser::DeclarationFileParser(Type declType,
    const std::map<std::string, IDeclarationCreator::Ptr>& creatorsByTypename) :
    _defaultDeclType(declType),
    _creatorsByTypename(creatorsByTypename)
{
    _defaultTypeCreator = getCreatorByType(declType);

    if (!_defaultTypeCreator)
    {
        throw std::invalid_argument("No creator has been associated to the default type " + getTypeName(declType));
    }
}

std::map<Type, NamedDeclarations>& DeclarationFileParser::getParsedDecls()
{
    return _parsedDecls;
}

const std::set<DeclarationFile>& DeclarationFileParser::getParsedFiles() const
{
    return _parsedFiles;
}

const std::vector<DeclarationBlockSyntax>& DeclarationFileParser::getUnrecognisedBlocks() const
{
    return _unrecognisedBlocks;
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
    }
}

void DeclarationFileParser::processBlock(IDeclarationCreator& creator, const DeclarationBlockSyntax& block)
{
    auto declaration = creator.createDeclaration(block.name);

    declaration->parseFromBlock(block);

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

}
