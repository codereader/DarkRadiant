#pragma once

#include "ideclmanager.h"
#include "icommandsystem.h"
#include <map>
#include <vector>
#include <memory>
#include <sigc++/connection.h>
#include "string/string.h"

#include "DeclarationFile.h"
#include "DeclarationFolderParser.h"

namespace decl
{

class DeclarationFolderParser;

class DeclarationManager :
    public IDeclarationManager
{
private:
    // Declaration names are compared case-insensitively
    using NamedDeclarations = std::map<std::string, IDeclaration::Ptr, string::ILess>;

    std::recursive_mutex _declarationAndCreatorLock;

    std::map<std::string, IDeclarationCreator::Ptr> _creatorsByTypename;
    std::map<Type, IDeclarationCreator::Ptr> _creatorsByType;

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

        std::future<void> parserFinisher;
        std::future<void> signalInvoker;
    };

    // One entry for each decl
    std::map<Type, Declarations> _declarationsByType;

    std::list<DeclarationBlockSyntax> _unrecognisedBlocks;
    std::recursive_mutex _unrecognisedBlockLock;

    std::map<Type, sigc::signal<void>> _declsReloadingSignals;
    std::map<Type, sigc::signal<void>> _declsReloadedSignals;
    std::mutex _signalAddLock; // mutex used to ensure only one thread is adding values to the above maps

    sigc::signal<void(Type, const std::string&, const std::string&)> _declRenamedSignal;
    sigc::signal<void(Type, const std::string&)> _declCreatedSignal;
    sigc::signal<void(Type, const std::string&)> _declRemovedSignal;

    std::size_t _parseStamp = 0;
    bool _reparseInProgress = false;

    // Holds the results during reparseDeclarations
    std::vector<std::pair<Type, ParseResult>> _parseResults;
    std::mutex _parseResultLock;

    sigc::connection _vfsInitialisedConn;

    // Access allowed if the _declarationAndCreatorLock is owned
    std::vector<std::shared_ptr<std::shared_future<void>>> _parserCleanupTasks;

public:
    void registerDeclType(const std::string& typeName, const IDeclarationCreator::Ptr& parser) override;
    void unregisterDeclType(const std::string& typeName) override;
    void registerDeclFolder(Type defaultType, const std::string& inputFolder, const std::string& inputExtension) override;
    IDeclaration::Ptr findDeclaration(Type type, const std::string& name) override;
    IDeclaration::Ptr findOrCreateDeclaration(Type type, const std::string& name) override;
    void foreachDeclaration(Type type, const std::function<void(const IDeclaration::Ptr&)>& functor) override;
    sigc::signal<void>& signal_DeclsReloading(Type type) override;
    sigc::signal<void>& signal_DeclsReloaded(Type type) override;
    sigc::signal<void(Type, const std::string&, const std::string&)>& signal_DeclRenamed() override;
    sigc::signal<void(Type, const std::string&)>& signal_DeclCreated() override;
    sigc::signal<void(Type, const std::string&)>& signal_DeclRemoved() override;
    void reloadDeclarations() override;
    bool renameDeclaration(Type type, const std::string& oldName, const std::string& newName) override;
    void removeDeclaration(Type type, const std::string& name) override;
    void saveDeclaration(const IDeclaration::Ptr& decl) override;

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    // Invoked once a parser thread has finished
    void onParserFinished(Type parserType, ParseResult& parsedBlocks);

private:
    void processParseResult(Type parserType, ParseResult& parsedBlocks);
    void runParsersForAllFolders();
    void waitForTypedParsersToFinish();
    void waitForCleanupTasksToFinish();
    void waitForSignalInvokersToFinish();

    // Attempts to resolve the block type of the given block, returns true on success, false otherwise.
    // Stores the determined type in the given reference.
    std::map<std::string, Type, string::ILess> getTypenameMapping();
    bool tryDetermineBlockType(const DeclarationBlockSyntax& block, Type& type);
    void processParsedBlocks(ParseResult& parsedBlocks);
    void removeDeclarationFromFile(const IDeclaration::Ptr& decl);

    // Requires the creatorsMutex and the declarationMutex to be locked
    const IDeclaration::Ptr& createOrUpdateDeclaration(Type type, const DeclarationBlockSyntax& block);
    void doWithDeclarationLock(Type type, const std::function<void(NamedDeclarations&)>& action);
    void handleUnrecognisedBlocks();
    void reloadDeclsCmd(const cmd::ArgumentList& args);

    // Requires the creatorsMutex to be locked
    std::string getTypenameByType(Type type);

    // Emit the reloaded signal
    void emitDeclsReloadedSignal(Type type);

    void onFilesystemInitialised();
};

}
