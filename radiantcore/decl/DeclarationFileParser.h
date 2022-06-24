#pragma once

#include "ideclmanager.h"
#include "DeclarationFile.h"

namespace decl
{

class DeclarationManager;

class DeclarationFileParser
{
private:
    Type _defaultDeclType;

    std::map<std::string, IDeclarationParser::Ptr> _parsersByTypename;
    IDeclarationParser::Ptr _defaultTypeParser;

    std::set<DeclarationFile> _parsedFiles;
    std::map<Type, NamedDeclarations> _parsedDecls;

    std::vector<DeclarationBlockSyntax> _unrecognisedBlocks;

public:
    DeclarationFileParser(Type declType, const std::map<std::string, IDeclarationParser::Ptr>& parsersByTypename);

    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir);

    std::map<Type, NamedDeclarations>& getParsedDecls();
    const std::set<DeclarationFile>& getParsedFiles() const;
    const std::vector<DeclarationBlockSyntax>& getUnrecognisedBlocks() const;

private:
    void parseBlock(IDeclarationParser& parser, const DeclarationBlockSyntax& block);
    IDeclarationParser::Ptr getParserByType(Type declType) const;
};

}
