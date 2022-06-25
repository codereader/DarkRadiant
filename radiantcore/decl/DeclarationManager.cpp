#include <future>

#include "DeclarationManager.h"

#include "DeclarationFolderParser.h"
#include "ifilesystem.h"
#include "module/StaticModule.h"
#include "string/trim.h"

namespace decl
{

void DeclarationManager::registerDeclType(const std::string& typeName, const IDeclarationCreator::Ptr& creator)
{
    {
        std::lock_guard<std::recursive_mutex> creatorLock(_creatorLock);

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
    std::lock_guard<std::recursive_mutex> parserLock(_creatorLock);

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

    std::lock_guard<std::recursive_mutex> declLock(_declarationLock);
    auto& decls = _declarationsByType.try_emplace(defaultType, Declarations()).first->second;

    // Start the parser thread
    decls.parser = std::make_unique<DeclarationFolderParser>(*this, defaultType, vfsPath, extension, getTypenameMapping());
    decls.parser->start();
}

std::map<std::string, Type> DeclarationManager::getTypenameMapping()
{
    std::map<std::string, Type> result;

    std::lock_guard<std::recursive_mutex> creatorLock(_creatorLock);

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

void DeclarationManager::reloadDecarations()
{
    std::lock_guard<std::recursive_mutex> fileLock(_parsedFileLock);
    std::lock_guard<std::recursive_mutex> parserLock(_creatorLock);

    for (const auto& [type, files] : _parsedFilesByDefaultType)
    {
        DeclarationFileParser parser(type, getTypenameMapping());

        for (const auto& file : files)
        {
            auto vfsFile = GlobalFileSystem().openTextFile(file.fullPath);

            if (!vfsFile) continue;

            auto fileInfo = GlobalFileSystem().getFileInfo(file.fullPath);

            try
            {
                // Parse entity defs from the file
                std::istream stream(&vfsFile->getInputStream());
                parser.parse(stream, fileInfo, vfsFile->getModName());
            }
            catch (parser::ParseException& e)
            {
                rError() << "[DeclParser] Failed to parse " << fileInfo.fullPath()
                    << " (" << e.what() << ")" << std::endl;
            }
        }

        // Submit the changes
        processParsedBlocks(parser.getParsedBlocks());
    }

    // Process the list of unrecognised blocks (from this and any previous run)
    handleUnrecognisedBlocks();

    // Invoke the declsReloaded signal for all types
    for (const auto& [type, files] : _parsedFilesByDefaultType)
    {
        signal_DeclsReloaded(type).emit();
    }
}

sigc::signal<void>& DeclarationManager::signal_DeclsReloaded(Type type)
{
    return _declsReloadedSignals.try_emplace(type).first->second;
}

void DeclarationManager::onParserFinished(Type parserType, 
    const std::map<Type, std::vector<DeclarationBlockSyntax>>& parsedBlocks,
    const std::set<DeclarationFile>& parsedFiles)
{
    // Sort all parsed files into the large list
    {
        std::lock_guard<std::recursive_mutex> lock(_parsedFileLock);

        // Merge all parsed files into the main set
        for (const auto& parsedFile : parsedFiles)
        {
            auto& fileSet = _parsedFilesByDefaultType.try_emplace(parsedFile.defaultDeclType, std::set<DeclarationFile>()).first->second;

            fileSet.emplace(parsedFile);
        }
    }

    // Sort all parsed blocks into our main dictionary
    // unrecognised blocks will be pushed to _unrecognisedBlocks
    processParsedBlocks(parsedBlocks);

    // Process the list of unrecognised blocks (from this and any previous run)
    handleUnrecognisedBlocks();

    // Invoke the declsReloaded signal for this type
    signal_DeclsReloaded(parserType).emit();
}

void DeclarationManager::processParsedBlocks(const std::map<Type, std::vector<DeclarationBlockSyntax>>& parsedBlocks)
{
    std::lock_guard<std::recursive_mutex> declLock(_declarationLock);
    std::lock_guard<std::recursive_mutex> creatorLock(_creatorLock);

    // Coming back from a parser thread, sort the parsed decls into the main dictionary
    for (auto& pair : parsedBlocks)
    {
        auto type = pair.first;

        for (const auto& block : pair.second)
        {
            if (type == Type::Undetermined)
            {
                // Block type unknown, put it on the pile, it will be processed later
                std::lock_guard<std::recursive_mutex> lock(_unrecognisedBlockLock);
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

void DeclarationManager::createOrUpdateDeclaration(Type type, const DeclarationBlockSyntax& block)
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
    else
    {
        // TODO: Compare parse stamps to see if this decl has duplicates
    }

    // Assign the block to the declaration instance
    existing->second->setBlockSyntax(block);
}

void DeclarationManager::handleUnrecognisedBlocks()
{
    std::lock_guard<std::recursive_mutex> lock(_unrecognisedBlockLock);

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
    _parsedFilesByDefaultType.clear();
    _unrecognisedBlocks.clear();
    _declarationsByType.clear();
    _creatorsByTypename.clear();
    _declsReloadedSignals.clear();
}

module::StaticModuleRegistration<DeclarationManager> _declManagerModule;

}
