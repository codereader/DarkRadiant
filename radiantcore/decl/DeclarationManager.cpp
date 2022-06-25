#include <future>

#include "DeclarationManager.h"

#include "DeclarationFolderParser.h"
#include "ifilesystem.h"
#include "module/StaticModule.h"
#include "string/trim.h"

namespace decl
{

void DeclarationManager::registerDeclType(const std::string& typeName, const IDeclarationCreator::Ptr& parser)
{
    std::lock_guard<std::mutex> parserLock(_creatorLock);

    if (_creatorsByTypename.count(typeName) > 0)
    {
        throw std::logic_error("Type name " + typeName + " has already been registered");
    }

    _creatorsByTypename.emplace(typeName, parser);

    // A new parser might be able to parse some of the unrecognised blocks
    handleUnrecognisedBlocks();
}

void DeclarationManager::unregisterDeclType(const std::string& typeName)
{
    std::lock_guard<std::mutex> parserLock(_creatorLock);

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

    _registeredFolders.emplace_back(RegisteredFolder{ vfsPath, extension, defaultType });

    std::lock_guard<std::mutex> declLock(_declarationLock);
    auto& decls = _declarationsByType.try_emplace(defaultType, Declarations()).first->second;

    // Start the parser thread
    decls.parser = std::make_unique<DeclarationFolderParser>(*this, defaultType, vfsPath, extension, _creatorsByTypename);
    decls.parser->start();
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

void DeclarationManager::foreachDeclaration(Type type, const std::function<void(const IDeclaration&)>& functor)
{
    doWithDeclarations(type, [&](const NamedDeclarations& decls)
    {
        for (const auto& [_, decl] : decls)
        {
            functor(*decl);
        }
    });
}

void DeclarationManager::doWithDeclarations(Type type, const std::function<void(const NamedDeclarations&)>& action)
{
    // Find type dictionary
    auto declLock = std::make_unique<std::lock_guard<std::mutex>>(_declarationLock);

    auto decls = _declarationsByType.find(type);

    if (decls == _declarationsByType.end()) return;

    // Ensure the parser is done
    if (decls->second.parser)
    {
        // Release the lock to give the thread a chance to finish
        declLock.reset();

        decls->second.parser->ensureFinished(); // blocks
        decls->second.parser.reset();

        declLock = std::make_unique<std::lock_guard<std::mutex>>(_declarationLock);
    }

    action(decls->second.decls);
}

void DeclarationManager::reloadDecarations()
{
    std::lock_guard<std::mutex> fileLock(_parsedFileLock);
    std::lock_guard<std::mutex> parserLock(_creatorLock);

    for (const auto& [type, files] : _parsedFilesByDefaultType)
    {
        DeclarationFileParser parser(type, _creatorsByTypename);

        for (const auto& file : files)
        {
            auto vfsFile = GlobalFileSystem().openTextFile(file.fullPath);

            if (!vfsFile) continue;

            // TODO
        }

        // Submit the changes
        // TODO
    }
}

sigc::signal<void>& DeclarationManager::signal_DeclsReloaded(Type type)
{
    return _declsReloadedSignals.try_emplace(type).first->second;
}

void DeclarationManager::onParserFinished(Type parserType, std::map<Type, NamedDeclarations>& parsedDecls,
    const std::vector<DeclarationBlockSyntax>& unrecognisedBlocks, const std::set<DeclarationFile>& parsedFiles)
{
    {
        std::lock_guard<std::mutex> declLock(_declarationLock);

        // Coming back from a parser thread, sort the parsed decls into the main dictionary
        for (auto& pair : parsedDecls)
        {
            auto& map = _declarationsByType.try_emplace(pair.first, Declarations()).first->second.decls;

            for (auto& parsedDecl : pair.second)
            {
                InsertDeclaration(map, std::move(parsedDecl.second));
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(_unrecognisedBlockLock);

        // Move all blocks into this one
        _unrecognisedBlocks.insert(_unrecognisedBlocks.end(),
            std::make_move_iterator(unrecognisedBlocks.begin()), std::make_move_iterator(unrecognisedBlocks.end()));
    }

    {
        std::lock_guard<std::mutex> lock(_parsedFileLock);

        // Merge all parsed files into the main set
        for (const auto& parsedFile : parsedFiles)
        {
            auto& fileSet = _parsedFilesByDefaultType.try_emplace(parsedFile.defaultDeclType, std::set<DeclarationFile>()).first->second;

            fileSet.emplace(parsedFile);
        }
    }

    // We might have received a parser in the meantime
    handleUnrecognisedBlocks();

    // Invoke the declsReloaded signal for this type
    signal_DeclsReloaded(parserType).emit();
}

void DeclarationManager::handleUnrecognisedBlocks()
{
    std::lock_guard<std::mutex> lock(_unrecognisedBlockLock);

    // Check each of the unrecognised blocks
    for (auto block = _unrecognisedBlocks.begin(); block != _unrecognisedBlocks.end();)
    {
        auto parser = _creatorsByTypename.find(block->typeName);

        if (parser == _creatorsByTypename.end())
        {
            ++block;
            continue; // still not recognised
        }

        // We found a parser, handle it now
        auto declaration = parser->second->createDeclaration(block->name);
        _unrecognisedBlocks.erase(block++);

        // Insert into our main library
        std::lock_guard<std::mutex> declLock(_declarationLock);

        auto& namedDecls = _declarationsByType.try_emplace(declaration->getDeclType(), Declarations()).first->second.decls;

        InsertDeclaration(namedDecls, std::move(declaration));
    }
}

void DeclarationManager::InsertDeclaration(NamedDeclarations& map, IDeclaration::Ptr&& declaration)
{
    auto type = declaration->getDeclType();
    auto result = map.try_emplace(declaration->getDeclName(), std::move(declaration));

    if (!result.second)
    {
        rWarning() << "[DeclParser]: " << getTypeName(type) << " " <<
            result.first->second->getDeclName() << " has already been declared" << std::endl;
    }
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
        MODULE_VIRTUALFILESYSTEM
    };

    return _dependencies;
}

void DeclarationManager::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;
}

void DeclarationManager::shutdownModule()
{
    auto declLock = std::make_unique<std::lock_guard<std::mutex>>(_declarationLock);
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
    _parsedFilesByDefaultType.clear();
    _unrecognisedBlocks.clear();
    _declarationsByType.clear();
    _creatorsByTypename.clear();
    _declsReloadedSignals.clear();
}

module::StaticModuleRegistration<DeclarationManager> _declManagerModule;

}
