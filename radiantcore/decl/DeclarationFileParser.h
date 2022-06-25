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

    std::map<std::string, IDeclarationCreator::Ptr> _creatorsByTypename;
    IDeclarationCreator::Ptr _defaultTypeCreator;

    std::set<DeclarationFile> _parsedFiles;
    std::map<Type, NamedDeclarations> _parsedDecls;

    std::vector<DeclarationBlockSyntax> _unrecognisedBlocks;

public:
    DeclarationFileParser(Type declType, const std::map<std::string, IDeclarationCreator::Ptr>& creatorsByTypename);

    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir);

    std::map<Type, NamedDeclarations>& getParsedDecls();
    const std::set<DeclarationFile>& getParsedFiles() const;
    const std::vector<DeclarationBlockSyntax>& getUnrecognisedBlocks() const;

private:
    void processBlock(IDeclarationCreator& creator, const DeclarationBlockSyntax& block);
    IDeclarationCreator::Ptr getCreatorByType(Type declType) const;
};

}
