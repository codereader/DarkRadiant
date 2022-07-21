#include <future>
#include <fstream>

#include "i18n.h"
#include "DeclarationManager.h"

#include <regex>

#include "DeclarationFolderParser.h"
#include "ifilesystem.h"
#include "decl/SpliceHelper.h"
#include "module/StaticModule.h"
#include "string/trim.h"
#include "os/path.h"
#include "fmt/format.h"
#include "stream/TemporaryOutputStream.h"
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

    doWithDeclarations(type, [&](NamedDeclarations& decls)
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

    doWithDeclarations(type, [&](NamedDeclarations& decls)
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
    doWithDeclarations(type, [&](NamedDeclarations& decls)
    {
        for (const auto& [_, decl] : decls)
        {
            functor(decl);
        }
    });
}

void DeclarationManager::doWithDeclarations(Type type, const std::function<void(NamedDeclarations&)>& action)
{
    // Find type dictionary
    auto declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationLock);

    auto decls = _declarationsByType.find(type);

    if (decls == _declarationsByType.end()) return;

    // Ensure the parser is done
    if (decls->second.parser)
    {
        // Move the parser to prevent the parser thread from trying to do
        // the same in onParserFinished
        std::unique_ptr parser(std::move(decls->second.parser));

        // Release the lock and let the thread finish
        declLock.reset();

        parser->ensureFinished(); // blocks
        parser.reset();

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

    // Process the buffered results synchronously
    for (auto& [type, result] : _parseResults)
    {
        processParseResult(type, result);
    }

    _parseResults.clear();

    // Empty all declarations that haven't been touched during this reparse run
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

    // Invoke the declsReloaded signal for all types
    for (const auto& [type, _] : _declarationsByType)
    {
        emitDeclsReloadedSignal(type);
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

void DeclarationManager::removeDeclaration(Type type, const std::string& name)
{
    // Acquire the lock and perform the removal
    doWithDeclarations(type, [&](NamedDeclarations& decls)
    {
        auto decl = decls.find(name);

        if (decl != decls.end())
        {
            decls.erase(decl);
        }
    });
}

bool DeclarationManager::renameDeclaration(Type type, const std::string& oldName, const std::string& newName)
{
    auto result = false;

    if (oldName == newName)
    {
        rWarning() << "Cannot rename, the new name is no different" << std::endl;
        return result;
    }

    // Acquire the lock and perform the removal
    doWithDeclarations(type, [&](NamedDeclarations& decls)
    {
        auto decl = decls.find(newName);
        
        if (decl != decls.end())
        {
            rWarning() << "Cannot rename declaration to " << newName << " since this name is already in use" << std::endl;
            return;
        }

        // Look up the original declaration
        decl = decls.find(oldName);

        if (decl == decls.end())
        {
            rWarning() << "Cannot rename non-existent declaration " << oldName << std::endl;
            return;
        }

        // Rename in definition table
        auto extracted = decls.extract(oldName);
        extracted.key() = newName;

        decls.insert(std::move(extracted));
        result = true;
    });

    return result;
}

namespace
{

void ensureTargetFileExists(const std::string& targetFile, const std::string& relativePath)
{
    // If the file doesn't exist yet, let's check if we need to inherit stuff from the VFS
    if (fs::exists(targetFile)) return;

    auto inheritFile = GlobalFileSystem().openTextFile(relativePath);

    if (!inheritFile) return;

    // There is a file with that name already in the VFS, copy it to the target file
    TextInputStream& inheritStream = inheritFile->getInputStream();

    std::ofstream outFile(targetFile);

    if (!outFile.is_open())
    {
        throw std::runtime_error(fmt::format(_("Cannot open file for writing: {0}"), targetFile));
    }

    char buf[16384];
    std::size_t bytesRead = inheritStream.read(buf, sizeof(buf));

    while (bytesRead > 0)
    {
        outFile.write(buf, bytesRead);

        bytesRead = inheritStream.read(buf, sizeof(buf));
    }

    outFile.close();
}

void writeDeclaration(std::ostream& stream, const IDeclaration::Ptr& decl)
{
    const auto& syntax = decl->getBlockSyntax();

    // Write the type (optional)
    if (!syntax.typeName.empty())
    {
        stream << syntax.typeName << " ";
    }

    // Write the decl name
    stream << decl->getDeclName() << std::endl;

    // Write the blocks including opening/closing braces
    stream << "{" << syntax.contents << "}";

    // A trailing line break after the declaration to keep distance
    stream << std::endl;
}

}

void DeclarationManager::saveDeclaration(const IDeclaration::Ptr& decl)
{
    const auto& syntax = decl->getBlockSyntax();

    // Check filename for emptiness
    if (syntax.fileInfo.name.empty())
    {
        throw std::invalid_argument("The file name of the decl is empty.");
    }

    // All parsers need to have finished
    waitForTypedParsersToFinish();

    std::string relativePath = syntax.fileInfo.fullPath();

    fs::path targetPath = GlobalGameManager().getModPath();

    if (targetPath.empty())
    {
        targetPath = GlobalGameManager().getUserEnginePath();

        rMessage() << "No mod base path found, falling back to user engine path to save particle file: " <<
            targetPath.string() << std::endl;
    }

    // Ensure the target folder exists
    targetPath /= os::getDirectory(relativePath);
    fs::create_directories(targetPath);

    auto targetFile = targetPath / os::getFilename(syntax.fileInfo.name);

    // Make sure the physical file exists and is inheriting its contents from the VFS (if necessary)
    ensureTargetFileExists(targetFile.string(), relativePath);

    // Open a temporary file
    stream::TemporaryOutputStream tempStream(targetFile);

    auto& stream = tempStream.getStream();

    // If a previous file exists, open it for reading and filter out the decl we'll be writing
    if (fs::exists(targetFile))
    {
        std::ifstream inheritStream(targetFile.string());

        if (!inheritStream.is_open())
        {
            throw std::runtime_error(fmt::format(_("Cannot open file for reading: {0}"), targetFile.string()));
        }

        // Look up the typename for this decl
        auto typeName = getTypenameByType(decl->getDeclType());

        // Write the file to the output stream, up to the point the decl should be written to
        // The typename is optional and compared case-sensitively
        std::regex pattern("^[\\s]*(" + typeName + "[\\s]+" + decl->getDeclName() + "|" + decl->getDeclName() + ")\\s*\\{*.*$", 
            std::regex_constants::icase);

        SpliceHelper::PipeStreamUntilInsertionPoint(inheritStream, stream, pattern);

        // We're at the insertion point (which might as well be EOF of the inheritStream)
        writeDeclaration(stream, decl);

        // Write the rest of the stream
        stream << inheritStream.rdbuf();

        inheritStream.close();
    }
    else
    {
        // Write the declaration itself
        writeDeclaration(stream, decl);
    }

    tempStream.closeAndReplaceTargetFile();
}

sigc::signal<void>& DeclarationManager::signal_DeclsReloading(Type type)
{
    return _declsReloadingSignals.try_emplace(type).first->second;
}

sigc::signal<void>& DeclarationManager::signal_DeclsReloaded(Type type)
{
    return _declsReloadedSignals.try_emplace(type).first->second;
}

void DeclarationManager::emitDeclsReloadedSignal(Type type)
{
    signal_DeclsReloaded(type).emit();
}

void DeclarationManager::onParserFinished(Type parserType, ParseResult& parsedBlocks)
{
    if (_reparseInProgress)
    {
        // In the reparse scenario the results are processed synchronously
        // so buffer everything and let the reloadDeclarations() method
        // assign the blocks in the thread that started it.
        auto& pair = _parseResults.emplace_back(parserType, ParseResult());
        pair.second.swap(parsedBlocks);
    }
    else
    {
        // When not reloading, process the result immediately
        processParseResult(parserType, parsedBlocks);
    }

    // Move the parser reference out and wait for it to finish. Once the parser is gone,
    // the public API will be available to any callbacks the decls reloaded signal
    // is going to invoke. Since we're already on the parser thread
    // itself here, we need to do this on a separate thread to avoid deadlocks
    {
        std::lock_guard declLock(_declarationLock);

        auto decls = _declarationsByType.find(parserType);
        assert(decls != _declarationsByType.end());

        // Check if the parser reference is still there,
        // it might have already been moved out in doWithDeclarations()
        if (decls->second.parser)
        {
            // Move the parser reference from the dictionary as capture to the lambda
            // Then let the unique_ptr in the lambda go out of scope to finish the thread
            // Lambda is mutable to make the unique_ptr member non-const
            decls->second.parserFinisher = std::async(std::launch::async, [p = std::move(decls->second.parser)]() mutable
            {
                p.reset();
            });
        }
    }

    // In the reparse scenario the calling code will emit this signal
    if (!_reparseInProgress)
    {
        // Emit the signal
        emitDeclsReloadedSignal(parserType);
    }
}

void DeclarationManager::processParseResult(Type parserType, ParseResult& parsedBlocks)
{
    // Sort all parsed blocks into our main dictionary
    // unrecognised blocks will be pushed to _unrecognisedBlocks
    processParsedBlocks(parsedBlocks);

    // Process the list of unrecognised blocks (from this and any previous run)
    handleUnrecognisedBlocks();
}

void DeclarationManager::processParsedBlocks(ParseResult& parsedBlocks)
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
