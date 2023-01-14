#include "DeclarationFolderParser.h"

#include "DeclarationManager.h"
#include "parser/DefBlockSyntaxParser.h"
#include "string/trim.h"

namespace decl
{

namespace
{
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
