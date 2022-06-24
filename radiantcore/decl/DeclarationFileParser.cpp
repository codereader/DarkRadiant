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
    const std::map<std::string, IDeclarationParser::Ptr>& parsersByTypename) :
    _defaultDeclType(declType),
    _parsersByTypename(parsersByTypename)
{
    _defaultTypeParser = getParserByType(declType);

    if (!_defaultTypeParser)
    {
        throw std::invalid_argument("No parser has been associated to the default type " + getTypeName(declType));
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
            // No type specified, use the default type parser
            parseBlock(*_defaultTypeParser, blockSyntax);
            continue;
        }

        // Locate a parser capable of handling that block
        auto parser = _parsersByTypename.find(block.name.substr(0, spacePos));

        if (parser != _parsersByTypename.end())
        {
            parseBlock(*parser->second, blockSyntax);
            continue;
        }

        // Unknown block type, move to buffer
        _unrecognisedBlocks.emplace_back(std::move(blockSyntax));
    }
}

void DeclarationFileParser::parseBlock(IDeclarationParser& parser, const DeclarationBlockSyntax& block)
{
    auto declaration = parser.parseFromBlock(block);

    auto& declMap = _parsedDecls.try_emplace(parser.getDeclType(), NamedDeclarations()).first->second;

    // Insert into map, emit a warning on duplicates
    DeclarationManager::InsertDeclaration(declMap, std::move(declaration));
}

IDeclarationParser::Ptr DeclarationFileParser::getParserByType(Type declType) const
{
    // Get the default type parser
    for (const auto& pair : _parsersByTypename)
    {
        if (pair.second->getDeclType() == declType)
        {
            return pair.second;
        }
    }

    return {};
}

}
