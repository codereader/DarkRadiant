#pragma once

#include "ideclmanager.h"
#include <map>
#include <vector>
#include <memory>
#include <thread>

namespace decl
{

class DeclarationParser;

class DeclarationManager :
    public IDeclarationManager
{
private:
    std::map<std::string, IDeclarationParser::Ptr> _parsersByTypename;

    struct RegisteredFolder
    {
        std::string folder;
        std::string extension;
        Type defaultType;
    };

    std::vector<RegisteredFolder> _registeredFolders;

    struct Declarations
    {
        // The decl library
        NamedDeclarations decls;

        // If not empty, holds the running parser
        std::unique_ptr<DeclarationParser> parser;
    };

    // One entry for each decl
    std::map<Type, Declarations> _declarationsByType;
    std::mutex _declarationLock;

    std::vector<DeclarationBlockSyntax> _unrecognisedBlocks;
    std::mutex _unrecognisedBlockLock;

public:
    void registerDeclType(const std::string& typeName, const IDeclarationParser::Ptr& parser) override;
    void unregisterDeclType(const std::string& typeName) override;
    void registerDeclFolder(Type defaultType, const std::string& inputFolder, const std::string& inputExtension) override;
    void foreachDeclaration(Type type, const std::function<void(const IDeclaration&)>& functor) override;

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;

    // Invoked once a parser thread has finished. It will move its data over to here.
    void onParserFinished(std::map<Type, NamedDeclarations>&& parsedDecls, 
        std::vector<DeclarationBlockSyntax>&& unrecognisedBlocks);
};

}
