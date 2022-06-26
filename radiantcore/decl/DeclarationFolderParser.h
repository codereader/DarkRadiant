#pragma once

#include "DeclarationFile.h"

#include "parser/ThreadedDeclParser.h"

namespace decl
{

class DeclarationManager;

// Threaded parser processing all files in the configured decl folder
// Submits all parsed declarations to the IDeclarationManager when finished
class DeclarationFolderParser :
    public parser::ThreadedDeclParser<void>
{
private:
    DeclarationManager& _owner;

    // Maps typename string ("material") to Type enum (Type::Material)
    std::map<std::string, Type> _typeMapping;

    // Holds all the identified blocks of all visited files
    std::map<Type, std::vector<DeclarationBlockSyntax>> _parsedBlocks;

    // The default type to assign to untyped blocks
    Type _defaultDeclType;

public:
    DeclarationFolderParser(DeclarationManager& owner, Type declType,
        const std::string& baseDir, const std::string& extension,
        const std::map<std::string, Type>& typeMapping);

protected:
    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override;
    void onFinishParsing() override;

private:
    Type determineBlockType(const DeclarationBlockSyntax& block);
};

}
