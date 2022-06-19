#pragma once

#include "ideclmanager.h"
#include <map>
#include <vector>
#include <memory>

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

    std::list<DeclarationBlockSyntax> _unrecognisedBlocks;
    std::mutex _unrecognisedBlockLock;

    std::map<Type, sigc::signal<void>> _declsReloadedSignals;

public:
    void registerDeclType(const std::string& typeName, const IDeclarationParser::Ptr& parser) override;
    void unregisterDeclType(const std::string& typeName) override;
    void registerDeclFolder(Type defaultType, const std::string& inputFolder, const std::string& inputExtension) override;
    IDeclaration::Ptr findDeclaration(Type type, const std::string& name) override;
    void foreachDeclaration(Type type, const std::function<void(const IDeclaration&)>& functor) override;
    sigc::signal<void>& signal_DeclsReloaded(Type type) override;

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    // Invoked once a parser thread has finished. It will move its data over to here.
    void onParserFinished(Type parserType, std::map<Type, NamedDeclarations>&& parsedDecls,
        std::vector<DeclarationBlockSyntax>&& unrecognisedBlocks);

    static void InsertDeclaration(NamedDeclarations& map, IDeclaration::Ptr&& declaration);

private:
    void doWithDeclarations(Type type, const std::function<void(const NamedDeclarations&)>& action);
    void handleUnrecognisedBlocks();
};

}
