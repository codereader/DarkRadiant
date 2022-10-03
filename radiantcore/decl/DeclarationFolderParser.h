#pragma once

#include <map>
#include "ideclmanager.h"
#include "DeclarationFile.h"

#include "parser/ThreadedDeclParser.h"
#include "string/string.h"

namespace decl
{

class DeclarationManager;

using ParseResult = std::map<Type, std::vector<DeclarationBlockSyntax>>;

// Threaded parser processing all files in the configured decl folder
// Submits all parsed declarations to the IDeclarationManager when finished
class DeclarationFolderParser :
    public parser::ThreadedDeclParser<void>
{
private:
    DeclarationManager& _owner;

    // Maps typename string ("material") to Type enum (Type::Material)
    std::map<std::string, Type, string::ILess> _typeMapping;

    // Holds all the identified blocks of all visited files
    ParseResult _parsedBlocks;

    // The default type to assign to untyped blocks
    Type _defaultDeclType;

public:
    DeclarationFolderParser(DeclarationManager& owner, Type declType,
        const std::string& baseDir, const std::string& extension,
        const std::map<std::string, Type, string::ILess>& typeMapping);

    ~DeclarationFolderParser() override
    {
        // Ensure that reset() is called while this class is still intact
        reset();
    }

protected:
    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override;
    void onFinishParsing() override;

private:
    Type determineBlockType(const DeclarationBlockSyntax& block);
};

}
