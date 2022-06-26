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

    std::map<std::string, Type> _typeMapping;

    std::map<Type, std::vector<DeclarationBlockSyntax>> _parsedBlocks;

public:
    DeclarationFileParser(Type defaultDeclType, const std::map<std::string, Type>& typeMapping);

    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir);

    std::map<Type, std::vector<DeclarationBlockSyntax>>& getParsedBlocks();

private:
    Type determineBlockType(const DeclarationBlockSyntax& block);
};

}
