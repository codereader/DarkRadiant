#pragma once

#include "DeclarationFile.h"
#include "DeclarationFileParser.h"
#include "ideclmanager.h"

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

    DeclarationFileParser _fileParser;
    Type _defaultDeclType;

public:
    DeclarationFolderParser(DeclarationManager& owner, Type declType,
        const std::string& baseDir, const std::string& extension,
        const std::map<std::string, IDeclarationParser::Ptr>& parsersByTypename);

protected:
    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override;
    void onFinishParsing() override;
};

}
