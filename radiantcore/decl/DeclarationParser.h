#pragma once

#include "ideclmanager.h"

#include "parser/DefBlockTokeniser.h"
#include "parser/ThreadedDeclParser.h"

namespace decl
{

class DeclarationManager;

class DeclarationParser :
    public parser::ThreadedDeclParser<void>
{
private:
    DeclarationManager& _owner;
    Type _defaultDeclType;

    std::map<std::string, IDeclarationParser::Ptr> _parsersByTypename;
    IDeclarationParser::Ptr _defaultTypeParser;

    std::map<Type, NamedDeclarations> _parsedDecls;

    std::vector<DeclarationBlockSyntax> _unrecognisedBlocks;

public:
    DeclarationParser(DeclarationManager& owner, Type declType, 
        const std::string& baseDir, const std::string& extension,
        const std::map<std::string, IDeclarationParser::Ptr>& parsersByTypename);

protected:
    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override;
    void onFinishParsing() override;

private:
    IDeclarationParser::Ptr getParserByType(Type declType) const;
    void parseBlock(IDeclarationParser& parser, const DeclarationBlockSyntax& blockSyntax);
};

}
