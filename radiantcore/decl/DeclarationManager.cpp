#include <future>

#include "DeclarationManager.h"

#include "DeclarationFolderParser.h"
#include "ifilesystem.h"
#include "module/StaticModule.h"
#include "string/trim.h"
#include "util/ScopedBoolLock.h"

namespace decl
{

void DeclarationManager::registerDeclType(const std::string& typeName, const IDeclarationCreator::Ptr& creator)
{
    {
        std::lock_guard creatorLock(_creatorLock);

        if (_creatorsByTypename.count(typeName) > 0 || _creatorsByType.count(creator->getDeclType()) > 0)
        {
            throw std::logic_error("Type name " + typeName + " and/or type " +
                getTypeName(creator->getDeclType()) + " has already been registered");
        }

        _creatorsByTypename.emplace(typeName, creator);
        _creatorsByType.emplace(creator->getDeclType(), creator);
    }

    // A new parser might be able to parse some of the unrecognised blocks
    handleUnrecognisedBlocks();
}

void DeclarationManager::unregisterDeclType(const std::string& typeName)
{
    std::lock_guard parserLock(_creatorLock);

    auto existing = _creatorsByTypename.find(typeName);

    if (existing == _creatorsByTypename.end())
    {
        throw std::logic_error("Type name " + typeName + " has not been registered");
    }

    _creatorsByTypename.erase(existing);
}

void DeclarationManager::registerDeclFolder(Type defaultType, const std::string& inputFolder, const std::string& inputExtension)
{
    // Sanitise input strings
    auto vfsPath = os::standardPathWithSlash(inputFolder);
    auto extension = string::trim_left_copy(inputExtension, ".");

    std::lock_guard folderLock(_registeredFoldersLock);
    _registeredFolders.emplace_back(RegisteredFolder{ vfsPath, extension, defaultType });

    std::lock_guard declLock(_declarationLock);
    auto& decls = _declarationsByType.try_emplace(defaultType, Declarations()).first->second;

    // Start the parser thread
    decls.parser = std::make_unique<DeclarationFolderParser>(*this, defaultType, vfsPath, extension, getTypenameMapping());
    decls.parser->start();
}

std::map<std::string, Type, string::ILess> DeclarationManager::getTypenameMapping()
{
    std::map<std::string, Type, string::ILess> result;

    std::lock_guard creatorLock(_creatorLock);

    for (const auto& [name, creator] : _creatorsByTypename)
    {
        result[name] = creator->getDeclType();
    }

    return result;
}

IDeclaration::Ptr DeclarationManager::findDeclaration(Type type, const std::string& name)
{
    IDeclaration::Ptr returnValue;

    doWithDeclarations(type, [&](const NamedDeclarations& decls)
    {
        auto decl = decls.find(name);

        if (decl != decls.end())
        {
            returnValue = decl->second;
        }
    });

    return returnValue;
}

IDeclaration::Ptr DeclarationManager::findOrCreateDeclaration(Type type, const std::string& name)
{
    IDeclaration::Ptr returnValue;

    doWithDeclarations(type, [&](const NamedDeclarations& decls)
    {
        auto decl = decls.find(name);

        if (decl != decls.end())
        {
            returnValue = decl->second;
            return;
        }

        // Nothing found, acquire the lock to create the new decl
        std::lock_guard creatorLock(_creatorLock);

        // Check if we even know that type, otherwise throw
        if (_creatorsByType.count(type) == 0)
        {
            throw std::invalid_argument("Unregistered type " + getTypeName(type));
        }

        // Construct the default block
        DeclarationBlockSyntax syntax;

        syntax.typeName = getTypenameByType(type);
        syntax.name = name;

        returnValue = createOrUpdateDeclaration(type, syntax);
    });

    // If the value is still empty at this point, throw
    if (!returnValue)
    {
        throw std::invalid_argument("Unregistered type " + getTypeName(type));
    }

    return returnValue;
}

void DeclarationManager::foreachDeclaration(Type type, const std::function<void(const IDeclaration::Ptr&)>& functor)
{
    doWithDeclarations(type, [&](const NamedDeclarations& decls)
    {
        for (const auto& [_, decl] : decls)
        {
            functor(decl);
        }
    });
}

void DeclarationManager::doWithDeclarations(Type type, const std::function<void(const NamedDeclarations&)>& action)
{
    // Find type dictionary
    auto declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationLock);

    auto decls = _declarationsByType.find(type);

    if (decls == _declarationsByType.end()) return;

    // Ensure the parser is done
    if (decls->second.parser)
    {
        // Release the lock to give the thread a chance to finish
        declLock.reset();

        decls->second.parser->ensureFinished(); // blocks
        decls->second.parser.reset();

        declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationLock);
    }

    action(decls->second.decls);
}

void DeclarationManager::reloadDeclarations()
{
    // Don't allow reloadDecls to be run before the startup phase is complete
    waitForTypedParsersToFinish();

    // Don't allow more than one simultaneous reloadDecls run
    if (_reparseInProgress) return;

    util::ScopedBoolLock reparseLock(_reparseInProgress);

    // Invoke the declsReloading signal for all types
    {
        std::lock_guard declLock(_declarationLock);

        for (const auto& [type, _] : _declarationsByType)
        {
            signal_DeclsReloading(type).emit();
        }
    }

    _parseStamp++;

    // Remove all unrecognised blocks from previous runs
    {
        std::lock_guard lock(_unrecognisedBlockLock);
        _unrecognisedBlocks.clear();
    }

    runParsersForAllFolders();

    // Empty all declarations that haven't been touched during this reparse run
    {
        std::lock_guard declLock(_declarationLock);

        for (const auto& [_, namedDecls] : _declarationsByType)
        {
            for (const auto& [name, decl] : namedDecls.decls)
            {
                if (decl->getParseStamp() < _parseStamp)
                {
                    rMessage() << "[DeclManager] " << getTypeName(decl->getDeclType()) << " " << 
                        name << " no longer present after reloadDecls" << std::endl;

                    auto syntax = decl->getBlockSyntax();

                    // Clear name and file info
                    syntax.contents.clear();
                    syntax.fileInfo = vfs::FileInfo();

                    decl->setBlockSyntax(syntax);
                }
            }
        }
    }

    // Invoke the declsReloaded signal for all types
    {
        std::lock_guard declLock(_declarationLock);

        for (const auto& [type, _] : _declarationsByType)
        {
            emitDeclsReloadedSignal(type, false);
        }
    }
}

void DeclarationManager::waitForTypedParsersToFinish()
{
    auto declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationLock);

    for (auto& [_, decls] : _declarationsByType)
    {
        if (!decls.parser) continue;

        // Release the lock to give the thread a chance to finish
        declLock.reset();

        decls.parser->ensureFinished(); // blocks
        decls.parser.reset();

        declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationLock);
    }
}

void DeclarationManager::runParsersForAllFolders()
{
    std::vector<std::unique_ptr<DeclarationFolderParser>> parsers;

    std::lock_guard folderLock(_registeredFoldersLock);

    auto typeMapping = getTypenameMapping();

    // Start a parser for each known folder
    for (const auto& folder : _registeredFolders)
    {
        auto& parser = parsers.emplace_back(
            std::make_unique<DeclarationFolderParser>(*this, folder.defaultType, folder.folder, folder.extension, typeMapping)
        );
        parser->start();
    }

    // Wait for all parsers to complete
    while (!parsers.empty())
    {
        parsers.back()->ensureFinished();
        parsers.pop_back();
    }
}

sigc::signal<void>& DeclarationManager::signal_DeclsReloading(Type type)
{
    return _declsReloadingSignals.try_emplace(type).first->second;
}

sigc::signal<void>& DeclarationManager::signal_DeclsReloaded(Type type)
{
    return _declsReloadedSignals.try_emplace(type).first->second;
}

void DeclarationManager::emitDeclsReloadedSignal(Type type, bool async)
{
    if (async)
    {
        std::thread([this, type]() { signal_DeclsReloaded(type).emit(); }).detach();
    }
    else
    {
        signal_DeclsReloaded(type).emit();
    }
}

void DeclarationManager::onParserFinished(Type parserType, 
    std::map<Type, std::vector<DeclarationBlockSyntax>>& parsedBlocks)
{
    // Sort all parsed blocks into our main dictionary
    // unrecognised blocks will be pushed to _unrecognisedBlocks
    processParsedBlocks(parsedBlocks);

    // Process the list of unrecognised blocks (from this and any previous run)
    handleUnrecognisedBlocks();

    // Invoke the declsReloaded signal for this type on a new thread
    // to allow clients to use the API without deadlocking
    emitDeclsReloadedSignal(parserType, true);
}

void DeclarationManager::processParsedBlocks(std::map<Type, std::vector<DeclarationBlockSyntax>>& parsedBlocks)
{
    std::lock_guard declLock(_declarationLock);
    std::lock_guard creatorLock(_creatorLock);

    // Coming back from a parser thread, sort the parsed decls into the main dictionary
    for (auto& [type, blocks] : parsedBlocks)
    {
        for (auto& block : blocks)
        {
            if (type == Type::Undetermined)
            {
                // Block type unknown, put it on the pile, it will be processed later
                std::lock_guard lock(_unrecognisedBlockLock);
                _unrecognisedBlocks.emplace_back(std::move(block));
                continue;
            }

            createOrUpdateDeclaration(type, block);
        }
    }
}

bool DeclarationManager::tryDetermineBlockType(const DeclarationBlockSyntax& block, Type& type)
{
    type = Type::Undetermined;

    if (block.typeName.empty())
    {
        return false;
    }

    auto creator = _creatorsByTypename.find(block.typeName);

    if (creator == _creatorsByTypename.end())
    {
        return false;
    }

    // Found a creator that can handle this type
    type = creator->second->getDeclType();
    return true;
}

const IDeclaration::Ptr& DeclarationManager::createOrUpdateDeclaration(Type type, const DeclarationBlockSyntax& block)
{
    // Get the mapping for this decl type
    auto& map = _declarationsByType.try_emplace(type, Declarations()).first->second.decls;

    // See if this decl is already in use
    auto existing = map.find(block.name);

    // Create declaration if not existing
    if (existing == map.end())
    {
        auto creator = _creatorsByType.at(type);
        existing = map.emplace(block.name, creator->createDeclaration(block.name)).first;
    }
    else if (existing->second->getParseStamp() == _parseStamp)
    {
        rWarning() << "[DeclParser]: " << getTypeName(type) << " " <<
            existing->second->getDeclName() << " has already been declared" << std::endl;

        // Any declaration following after the first is ignored
        return existing->second;
    }

    // Assign the block to the declaration instance
    existing->second->setBlockSyntax(block);

    // Update the parse stamp for this instance
    existing->second->setParseStamp(_parseStamp);

    return existing->second;
}

void DeclarationManager::handleUnrecognisedBlocks()
{
    std::lock_guard lock(_unrecognisedBlockLock);

    if (_unrecognisedBlocks.empty()) return;

    for (auto block = _unrecognisedBlocks.begin(); block != _unrecognisedBlocks.end();)
    {
        auto type = Type::Undetermined;

        if (!tryDetermineBlockType(*block, type))
        {
            ++block;
            continue;
        }

        createOrUpdateDeclaration(type, *block);
        _unrecognisedBlocks.erase(block++);
    }
}

std::string DeclarationManager::getTypenameByType(Type type)
{
    auto creator = _creatorsByType.at(type);

    for (const auto& [typeName, candidate] : _creatorsByTypename)
    {
        if (candidate == creator)
        {
            return typeName;
        }
    }

    throw std::invalid_argument("Unregistered type: " + getTypeName(type));
}

const std::string& DeclarationManager::getName() const
{
    static std::string _name(MODULE_DECLMANAGER);
    return _name;
}

const StringSet& DeclarationManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_VIRTUALFILESYSTEM,
        MODULE_COMMANDSYSTEM,
    };

    return _dependencies;
}

void DeclarationManager::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    GlobalCommandSystem().addCommand("ReloadDecls",
        std::bind(&DeclarationManager::reloadDeclsCmd, this, std::placeholders::_1));

    // After the initial parsing, all decls will have a parseStamp of 0
    _parseStamp = 0;
    _reparseInProgress = false;
}

void DeclarationManager::shutdownModule()
{
    auto declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationLock);
    std::vector<std::unique_ptr<DeclarationFolderParser>> parsersToFinish;

    for (auto& [_, decl] : _declarationsByType)
    {
        if (decl.parser)
        {
            parsersToFinish.emplace_back(std::move(decl.parser));
        }
    }

    // Release the lock, destruct all parsers
    declLock.reset();
    parsersToFinish.clear(); // might block if parsers are running

    // All parsers have finished, clear the structure, no need to lock anything
    _registeredFolders.clear();
    _unrecognisedBlocks.clear();
    _declarationsByType.clear();
    _creatorsByTypename.clear();
    _declsReloadingSignals.clear();
    _declsReloadedSignals.clear();
}

void DeclarationManager::reloadDeclsCmd(const cmd::ArgumentList& _)
{
    reloadDeclarations();
}

module::StaticModuleRegistration<DeclarationManager> _declManagerModule;

}
