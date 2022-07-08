#pragma once

#include "ideclmanager.h"
#include "icommandsystem.h"
#include <map>
#include <set>
#include <vector>
#include <memory>
#include "string/string.h"

#include "DeclarationFile.h"

namespace decl
{

class DeclarationFolderParser;

class DeclarationManager :
    public IDeclarationManager
{
private:
    // Declaration names are compared case-insensitively
    using NamedDeclarations = std::map<std::string, IDeclaration::Ptr, string::ILess>;

    std::map<std::string, IDeclarationCreator::Ptr> _creatorsByTypename;
    std::map<Type, IDeclarationCreator::Ptr> _creatorsByType;
    std::recursive_mutex _creatorLock;

    struct RegisteredFolder
    {
        std::string folder;
        std::string extension;
        Type defaultType;
    };

    std::vector<RegisteredFolder> _registeredFolders;
    std::recursive_mutex _registeredFoldersLock;

    struct Declarations
    {
        // The decl library
        NamedDeclarations decls;

        // If not empty, holds the running parser
        std::unique_ptr<DeclarationFolderParser> parser;
    };

    // One entry for each decl
    std::map<Type, Declarations> _declarationsByType;
    std::recursive_mutex _declarationLock;

    std::list<DeclarationBlockSyntax> _unrecognisedBlocks;
    std::recursive_mutex _unrecognisedBlockLock;

    std::map<Type, sigc::signal<void>> _declsReloadingSignals;
    std::map<Type, sigc::signal<void>> _declsReloadedSignals;

    std::size_t _parseStamp = 0;
    bool _reparseInProgress = false;

public:
    void registerDeclType(const std::string& typeName, const IDeclarationCreator::Ptr& parser) override;
    void unregisterDeclType(const std::string& typeName) override;
    void registerDeclFolder(Type defaultType, const std::string& inputFolder, const std::string& inputExtension) override;
    IDeclaration::Ptr findDeclaration(Type type, const std::string& name) override;
    IDeclaration::Ptr findOrCreateDeclaration(Type type, const std::string& name) override;
    void foreachDeclaration(Type type, const std::function<void(const IDeclaration::Ptr&)>& functor) override;
    sigc::signal<void>& signal_DeclsReloading(Type type) override;
    sigc::signal<void>& signal_DeclsReloaded(Type type) override;
    void reloadDeclarations() override;

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    // Invoked once a parser thread has finished. It will move its data over to here.
    void onParserFinished(Type parserType,
        std::map<Type, std::vector<DeclarationBlockSyntax>>& parsedBlocks);

private:
    void runParsersForAllFolders();
    void waitForTypedParsersToFinish();

    // Attempts to resolve the block type of the given block, returns true on success, false otherwise.
    // Stores the determined type in the given reference.
    std::map<std::string, Type, string::ILess> getTypenameMapping();
    bool tryDetermineBlockType(const DeclarationBlockSyntax& block, Type& type);
    void processParsedBlocks(std::map<Type, std::vector<DeclarationBlockSyntax>>& parsedBlocks);

    // Requires the creatorsMutex and the declarationMutex to be locked
    const IDeclaration::Ptr& createOrUpdateDeclaration(Type type, const DeclarationBlockSyntax& block);
    void doWithDeclarations(Type type, const std::function<void(const NamedDeclarations&)>& action);
    void handleUnrecognisedBlocks();
    void reloadDeclsCmd(const cmd::ArgumentList& args);

    // Requires the creatorsMutex to be locked
    std::string getTypenameByType(Type type);

    // Emit the reloaded signal (on a different thread if requested)
    void emitDeclsReloadedSignal(Type type, bool async);
};

}
