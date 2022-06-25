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

#if 0
    std::map<std::string, IDeclarationCreator::Ptr> _creatorsByTypename;
    IDeclarationCreator::Ptr _defaultTypeCreator;
#endif
    std::map<std::string, Type> _typeMapping;

    std::set<DeclarationFile> _parsedFiles;
    std::map<Type, std::vector<DeclarationBlockSyntax>> _parsedBlocks;

#if 0
    std::map<Type, NamedDeclarations> _parsedDecls;

    std::vector<DeclarationBlockSyntax> _unrecognisedBlocks;
#endif

public:
    DeclarationFileParser(Type defaultDeclType, const std::map<std::string, Type>& typeMapping);

    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir);

    std::map<Type, std::vector<DeclarationBlockSyntax>>& getParsedBlocks();
    const std::set<DeclarationFile>& getParsedFiles() const;

private:
    Type determineBlockType(const DeclarationBlockSyntax& block);
    void processBlock(IDeclarationCreator& creator, const DeclarationBlockSyntax& block);
    IDeclarationCreator::Ptr getCreatorByType(Type declType) const;
};

}
