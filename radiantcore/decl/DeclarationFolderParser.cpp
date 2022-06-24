#include "DeclarationFolderParser.h"

#include "DeclarationManager.h"

namespace decl
{

DeclarationFolderParser::DeclarationFolderParser(DeclarationManager& owner, Type declType, 
    const std::string& baseDir, const std::string& extension,
    const std::map<std::string, IDeclarationParser::Ptr>& parsersByTypename) :
    ThreadedDeclParser<void>(declType, baseDir, extension, 1),
    _owner(owner),
    _fileParser(declType, parsersByTypename),
    _defaultDeclType(declType)
{}

void DeclarationFolderParser::parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir)
{
    _fileParser.parse(stream, fileInfo, modDir);
}

void DeclarationFolderParser::onFinishParsing()
{
    // Submit all parsed declarations to the decl manager
    _owner.onParserFinished(_defaultDeclType, _fileParser.getParsedDecls(), _fileParser.getUnrecognisedBlocks(), _fileParser.getParsedFiles());
}

}
