#include "DeclarationFolderParser.h"

#include "DeclarationManager.h"
#include "parser/DefBlockTokeniser.h"
#include "parser/DefBlockSyntaxParser.h"
#include "string/trim.h"

namespace decl
{

namespace
{
    std::string getBlockTypeName(const std::string& declaration)
    {
        auto spacePos = declaration.find(' ');

        if (spacePos == std::string::npos) return {};

        return string::trim_copy(declaration.substr(0, spacePos), " \t"); // remove all tabs and spaces
    }

    DeclarationBlockSyntax createBlock(const parser::BlockTokeniser::Block& block,
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

    DeclarationBlockSyntax createBlock(const parser::DefBlockSyntax& block,
        const vfs::FileInfo& fileInfo, const std::string& modName)
    {
        DeclarationBlockSyntax syntax;

        const auto& nameSyntax = block.getName();
        const auto& typeSyntax = block.getType();

        syntax.typeName = typeSyntax ? typeSyntax->getToken().value : "";
        syntax.name = nameSyntax ? nameSyntax->getToken().value : "";
        syntax.contents = block.getBlockContents();
        syntax.modName = modName;
        syntax.fileInfo = fileInfo;

        return syntax;
    }
}

DeclarationFolderParser::DeclarationFolderParser(DeclarationManager& owner, Type declType, 
    const std::string& baseDir, const std::string& extension,
    const std::map<std::string, Type, string::ILess>& typeMapping) :
    ThreadedDeclParser<void>(declType, baseDir, extension, 1),
    _owner(owner),
    _typeMapping(typeMapping),
    _defaultDeclType(declType)
{}

void DeclarationFolderParser::parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir)
{
#if 1
    // Parse the incoming stream into syntax blocks
    parser::DefBlockSyntaxParser<std::istream> parser(stream);

    auto syntaxTree = parser.parse();

    for (const auto& node : syntaxTree->getRoot()->getChildren())
    {
        if (node->getType() != parser::DefSyntaxNode::Type::DeclBlock)
        {
            continue;
        }

        const auto& blockNode = static_cast<const parser::DefBlockSyntax&>(*node);

        // Convert the incoming block to a DeclarationBlockSyntax
        auto blockSyntax = createBlock(blockNode, fileInfo, modDir);

        // Move the block in the correct bucket
        auto declType = determineBlockType(blockSyntax);
        auto& blockList = _parsedBlocks.try_emplace(declType).first->second;
        blockList.emplace_back(std::move(blockSyntax));
    }
#else
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
    }
#endif
}

void DeclarationFolderParser::onFinishParsing()
{
    // Submit all parsed declarations to the decl manager
    _owner.onParserFinished(_defaultDeclType, _parsedBlocks);
}

Type DeclarationFolderParser::determineBlockType(const DeclarationBlockSyntax& block)
{
    if (block.typeName.empty())
    {
        return _defaultDeclType;
    }

    auto foundType = _typeMapping.find(block.typeName);

    return foundType != _typeMapping.end() ? foundType->second : Type::Undetermined;
}

}
