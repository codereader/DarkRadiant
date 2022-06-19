#include <future>

#include "DeclarationManager.h"

#include "ifilesystem.h"
#include "module/StaticModule.h"
#include "string/trim.h"
#include "DeclarationParser.h"

namespace decl
{

void DeclarationManager::registerDeclType(const std::string& typeName, const IDeclarationParser::Ptr& parser)
{
    if (_parsersByTypename.count(typeName) > 0)
    {
        throw std::logic_error("Type name " + typeName + " has already been registered");
    }

    _parsersByTypename.emplace(typeName, parser);
}

void DeclarationManager::unregisterDeclType(const std::string& typeName)
{
    auto existing = _parsersByTypename.find(typeName);

    if (existing == _parsersByTypename.end())
    {
        throw std::logic_error("Type name " + typeName + " has not been registered");
    }

    _parsersByTypename.erase(existing);
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
    decls.parser = std::make_unique<DeclarationParser>(*this, defaultType, vfsPath, extension, _parsersByTypename);
    decls.parser->start();
}

void DeclarationManager::foreachDeclaration(Type type, const std::function<void(const IDeclaration&)>& functor)
{
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

    for (const auto& [_, decl] : decls->second.decls)
    {
        functor(*decl);
    }
}

void DeclarationManager::onParserFinished(std::map<Type, NamedDeclarations>&& parsedDecls,
    std::vector<DeclarationBlockSyntax>&& unrecognisedBlocks)
{
    {
        std::lock_guard<std::mutex> declLock(_declarationLock);

        // Coming back from a parser thread, sort the parsed decls into the main dictionary
        for (auto& pair : parsedDecls)
        {
            auto& map = _declarationsByType.try_emplace(pair.first, Declarations()).first->second.decls;

            map.merge(pair.second);
        }
    }

    {
        std::lock_guard<std::mutex> lock(_unrecognisedBlockLock);

        // Move all blocks into this one
        _unrecognisedBlocks.insert(_unrecognisedBlocks.end(),
            std::make_move_iterator(unrecognisedBlocks.begin()), std::make_move_iterator(unrecognisedBlocks.end()));
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
    std::vector<std::unique_ptr<DeclarationParser>> parsersToFinish;

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
    _declarationsByType.clear();
}

module::StaticModuleRegistration<DeclarationManager> _declManagerModule;

}
