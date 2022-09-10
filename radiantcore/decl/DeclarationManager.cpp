#include <future>
#include <fstream>

#include "i18n.h"
#include "DeclarationManager.h"

#include <regex>

#include "DeclarationFolderParser.h"
#include "parser/DefBlockSyntaxParser.h"
#include "ifilesystem.h"
#include "module/StaticModule.h"
#include "string/trim.h"
#include "os/path.h"
#include "os/file.h"
#include "fmt/format.h"
#include "gamelib.h"
#include "stream/TemporaryOutputStream.h"
#include "util/ScopedBoolLock.h"

namespace decl
{

void DeclarationManager::registerDeclType(const std::string& typeName, const IDeclarationCreator::Ptr& creator)
{
    {
        std::lock_guard creatorLock(_declarationAndCreatorLock);

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
    std::lock_guard parserLock(_declarationAndCreatorLock);

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

    {
        std::lock_guard folderLock(_registeredFoldersLock);
        _registeredFolders.emplace_back(RegisteredFolder{ vfsPath, extension, defaultType });
    }

    std::lock_guard declLock(_declarationAndCreatorLock);
    auto& decls = _declarationsByType.try_emplace(defaultType, Declarations()).first->second;

    // Start the parser thread
    decls.parser = std::make_unique<DeclarationFolderParser>(*this, defaultType, vfsPath, extension, getTypenameMapping());
    decls.parser->start();
}

std::map<std::string, Type, string::ILess> DeclarationManager::getTypenameMapping()
{
    std::map<std::string, Type, string::ILess> result;

    std::lock_guard creatorLock(_declarationAndCreatorLock);

    for (const auto& [name, creator] : _creatorsByTypename)
    {
        result[name] = creator->getDeclType();
    }

    return result;
}

IDeclaration::Ptr DeclarationManager::findDeclaration(Type type, const std::string& name)
{
    IDeclaration::Ptr returnValue;

    doWithDeclarationLock(type, [&](NamedDeclarations& decls)
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

    doWithDeclarationLock(type, [&](NamedDeclarations& decls)
    {
        auto decl = decls.find(name);

        if (decl != decls.end())
        {
            returnValue = decl->second;
            return;
        }

        // Nothing found, acquire the lock to create the new decl
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
    doWithDeclarationLock(type, [&](NamedDeclarations& decls)
    {
        for (const auto& [_, decl] : decls)
        {
            functor(decl);
        }
    });
}

void DeclarationManager::doWithDeclarationLock(Type type, const std::function<void(NamedDeclarations&)>& action)
{
    // Find type dictionary
    auto declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationAndCreatorLock);

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

        declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationAndCreatorLock);
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
        std::lock_guard declLock(_declarationAndCreatorLock);

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

    {
        std::lock_guard lock(_parseResultLock);

        // Process the buffered results synchronously
        for (auto& [type, result] : _parseResults)
        {
            processParseResult(type, result);
        }

        _parseResults.clear();
    }

    // Empty all declarations that haven't been touched during this reparse run
    std::lock_guard declLock(_declarationAndCreatorLock);

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
    auto declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationAndCreatorLock);

    for (auto& [_, decls] : _declarationsByType)
    {
        if (!decls.parser) continue;

        // Move the parser to prevent the parser thread from trying to do
        // the same in onParserFinished
        std::unique_ptr parser(std::move(decls.parser));

        // Release the lock and let the thread finish
        declLock.reset();

        parser->ensureFinished(); // blocks
        parser.reset();

        declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationAndCreatorLock);
    }
}

void DeclarationManager::runParsersForAllFolders()
{
    std::vector<std::unique_ptr<DeclarationFolderParser>> parsers;

    {
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
    // All parsers need to have finished
    waitForTypedParsersToFinish();

    // Acquire the lock and perform the removal
    doWithDeclarationLock(type, [&](NamedDeclarations& decls)
    {
        auto decl = decls.find(name);

        if (decl != decls.end())
        {
            removeDeclarationFromFile(decl->second);

            // Clear out this declaration's syntax block
            auto syntax = decl->second->getBlockSyntax();
            syntax.name.clear();
            syntax.typeName.clear();
            syntax.contents.clear();
            syntax.fileInfo = vfs::FileInfo();
            decl->second->setBlockSyntax(syntax);

            decls.erase(decl);
        }
    });
}

namespace
{

bool removeDeclarationFromSyntaxTree(const parser::DefSyntaxTree::Ptr& syntaxTree, const std::string& declName)
{
    // Remove the declaration from the tree
    std::vector<parser::DefSyntaxNode::Ptr> nodesToRemove;

    const auto& childNodes = syntaxTree->getRoot()->getChildren();
    for (int i = 0; i < childNodes.size(); ++i)
    {
        const auto& node = childNodes.at(i);

        if (node->getType() != parser::DefSyntaxNode::Type::DeclBlock) continue;

        auto blockNode = std::static_pointer_cast<parser::DefBlockSyntax>(node);

        if (blockNode->getName() && blockNode->getName()->getToken().value == declName)
        {
            nodesToRemove.push_back(blockNode);

            parser::DefSyntaxNode::Ptr encounteredWhitespace;

            // Try to locate comment nodes preceding this block
            for (int p = i - 1; p >= 0; --p)
            {
                const auto& predecessor = childNodes.at(p);
                if (predecessor->getType() == parser::DefSyntaxNode::Type::Whitespace)
                {
                    // Stop at the first whitespace token that contains more than one line break
                    auto whitespace = std::static_pointer_cast<parser::DefWhitespaceSyntax>(predecessor);
                    
                    // stop searching at the first empty line
                    if (whitespace->getNumberOfLineBreaks() > 1) break;

                    // This is just a single line break (or none), remember to remove it when we encounter a comment
                    encounteredWhitespace = predecessor;
                }
                else if (predecessor->getType() == parser::DefSyntaxNode::Type::Comment)
                {
                    nodesToRemove.push_back(predecessor);

                    // Remove any whitespace that stood between this comment and the block
                    if (encounteredWhitespace)
                    {
                        nodesToRemove.push_back(encounteredWhitespace);
                        encounteredWhitespace.reset();
                    }

                    continue;
                }
                else // stop at all other node types
                {
                    break;
                }
            }
        }
    }

    for (const auto& node : nodesToRemove)
    {
        syntaxTree->getRoot()->removeChildNode(node);
    }

    return !nodesToRemove.empty(); // true if we removed one node or more
}

}

void DeclarationManager::removeDeclarationFromFile(const IDeclaration::Ptr& decl)
{
    const auto& syntax = decl->getBlockSyntax();

    // Nothing to do if the decl hasn't been saved
    if (syntax.fileInfo.name.empty()) return;

    if (!syntax.fileInfo.getIsPhysicalFile())
    {
        throw std::logic_error("Only declarations stored in physical files can be removed.");
    }

    auto fullPath = GlobalFileSystem().findFile(syntax.fileInfo.fullPath());

    if (fullPath.empty() || !fs::exists(fullPath))
    {
        return;
    }

    fullPath += syntax.fileInfo.fullPath();

    // Load the syntax tree from the existing file
    std::ifstream existingFile(fullPath);

    if (!existingFile.is_open())
    {
        throw std::runtime_error(fmt::format(_("Cannot open file for reading: {0}"), fullPath));
    }

    // Parse the existing file into a syntax tree for manipulation
    parser::DefBlockSyntaxParser<std::istream> parser(existingFile);
    
    auto syntaxTree = parser.parse();
    existingFile.close();

    // Try to remove the decl from the syntax tree (using the original name)
    // Returns false if the decl could not be located (decl has not been saved to this file then)
    if (removeDeclarationFromSyntaxTree(syntaxTree, decl->getOriginalDeclName()))
    {
        // Open a temporary file
        stream::TemporaryOutputStream tempStream(fullPath);
        auto& stream = tempStream.getStream();

        // Move the old file to .bak before overwriting it
        os::moveToBackupFile(fullPath);

        // Export the modified syntax tree
        stream << syntaxTree->getString();

        tempStream.closeAndReplaceTargetFile();
    }
}

bool DeclarationManager::renameDeclaration(Type type, const std::string& oldName, const std::string& newName)
{
    auto result = false;

    if (oldName == newName)
    {
        rWarning() << "Cannot rename, the new name is no different" << std::endl;
        return result;
    }

    // Acquire the lock and perform the rename
    doWithDeclarationLock(type, [&](NamedDeclarations& decls)
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

        decl = decls.insert(std::move(extracted)).position;

        // Store the new in the decl itself
        decl->second->setDeclName(newName);

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

    fs::path targetPath = game::current::getWriteableGameResourcePath();

    // Ensure the target folder exists
    targetPath /= os::getDirectory(relativePath);
    fs::create_directories(targetPath);

    auto targetFile = targetPath / os::getFilename(syntax.fileInfo.name);

    // Make sure the physical file exists and is inheriting its contents from the VFS (if necessary)
    ensureTargetFileExists(targetFile.string(), relativePath);

    // Open a temporary file
    stream::TemporaryOutputStream tempStream(targetFile);

    auto& stream = tempStream.getStream();

    parser::DefSyntaxTree::Ptr syntaxTree;

    if (fs::exists(targetFile))
    {
        // Parse the existing file into a syntax tree for manipulation
        std::ifstream inheritStream(targetFile.string());

        parser::DefBlockSyntaxParser<std::istream> parser(inheritStream);

        syntaxTree = parser.parse();
        inheritStream.close();
    }
    else
    {
        // Since there's no existing file, create a new syntax tree
        syntaxTree = std::make_shared<parser::DefSyntaxTree>();
    }

    // Take the first named block matching our decl. There's a risk that there is another
    // decl of a different type with the same name in the tree, but we don't try to check this here
    auto block = syntaxTree->findFirstNamedBlock(decl->getOriginalDeclName());

    if (!block)
    {
        // Create a new block node with leading whitespace and add it to the tree
        syntaxTree->getRoot()->appendChildNode(parser::DefWhitespaceSyntax::Create("\n\n"));

        block = parser::DefBlockSyntax::CreateTypedBlock(syntax.typeName, decl->getDeclName());
        syntaxTree->getRoot()->appendChildNode(block);
    }

    // Check if the name of the decl has been changed
    if (decl->getDeclName() != decl->getOriginalDeclName())
    {
        // Update the name syntax node
        block->getName()->setName(decl->getDeclName());
    }

    // Store the new block contents and save the file
    block->setBlockContents(syntax.contents);

    // Export the modified syntax tree
    stream << syntaxTree->getString();

    tempStream.closeAndReplaceTargetFile();

    // Refresh the file info, otherwise a newly created file might not be considered "physical"
    // and declarations might report themselves as if they were originating in a PK4
    decl->setFileInfo(GlobalFileSystem().getFileInfo(relativePath));

    if (decl->getDeclName() != decl->getOriginalDeclName())
    {
        // Now that the decl is saved under a new name, update the original decl name
        decl->setOriginalDeclName(decl->getDeclName());
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

void DeclarationManager::emitDeclsReloadedSignal(Type type)
{
    signal_DeclsReloaded(type).emit();
}

void DeclarationManager::onParserFinished(Type parserType, ParseResult& parsedBlocks)
{
    if (_reparseInProgress)
    {
        std::lock_guard lock(_parseResultLock);

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
        std::lock_guard declLock(_declarationAndCreatorLock);

        auto decls = _declarationsByType.find(parserType);
        assert(decls != _declarationsByType.end());

        // Check if the parser reference is still there,
        // it might have already been moved out in doWithDeclarationLock()
        if (decls->second.parser)
        {
            // Move parser ptr as long as we hold the lock
            std::unique_ptr parser(std::move(decls->second.parser));

            // Move the parser reference from the local variable as capture to the lambda
            // Then let the unique_ptr in the lambda go out of scope to finish the thread
            // Lambda is mutable to make the unique_ptr member non-const
            decls->second.parserFinisher = std::async(std::launch::async, [p = std::move(parser)]() mutable
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
    std::vector<DeclarationBlockSyntax> unrecognisedBlocks;

    {
        std::lock_guard declLock(_declarationAndCreatorLock);

        // Coming back from a parser thread, sort the parsed decls into the main dictionary
        for (auto& [type, blocks] : parsedBlocks)
        {
            for (auto& block : blocks)
            {
                if (type == Type::Undetermined)
                {
                    // Block type unknown, put it on the pile, it will be processed later
                    unrecognisedBlocks.emplace_back(std::move(block));
                    continue;
                }

                createOrUpdateDeclaration(type, block);
            }
        }
    }

    // With the _declarationLock and _creatorLock released, push blocks to the pile of unrecognised ones
    std::lock_guard lock(_unrecognisedBlockLock);
    _unrecognisedBlocks.insert(_unrecognisedBlocks.end(), std::make_move_iterator(unrecognisedBlocks.begin()),
        std::make_move_iterator(unrecognisedBlocks.end()));
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
    auto it = _declarationsByType.find(type);

    if (it == _declarationsByType.end())
    {
        it = _declarationsByType.emplace(type, Declarations()).first;
    }

    auto& map = it->second.decls;

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
    auto unrecognisedBlockLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_unrecognisedBlockLock);

    if (_unrecognisedBlocks.empty()) return;

    // Move all unrecognised blocks to a temporary structure and release the lock
    std::list<DeclarationBlockSyntax> unrecognisedBlocks(std::move(_unrecognisedBlocks));
    unrecognisedBlockLock.reset();

    {
        std::lock_guard declarationLock(_declarationAndCreatorLock);

        for (auto block = unrecognisedBlocks.begin(); block != unrecognisedBlocks.end();)
        {
            auto type = Type::Undetermined;

            if (!tryDetermineBlockType(*block, type))
            {
                ++block;
                continue;
            }

            createOrUpdateDeclaration(type, *block);

            unrecognisedBlocks.erase(block++);
        }
    }

    // All remaining unrecognised blocks are moved back to the pile
    unrecognisedBlockLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_unrecognisedBlockLock);
    _unrecognisedBlocks.insert(_unrecognisedBlocks.end(), std::move_iterator(unrecognisedBlocks.begin()),
        std::move_iterator(unrecognisedBlocks.end()));
}

std::string DeclarationManager::getTypenameByType(Type type)
{
    std::lock_guard declarationLock(_declarationAndCreatorLock);

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

void DeclarationManager::onFilesystemInitialised()
{
    reloadDeclarations();
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
    GlobalCommandSystem().addCommand("ReloadDecls",
        std::bind(&DeclarationManager::reloadDeclsCmd, this, std::placeholders::_1));

    // After the initial parsing, all decls will have a parseStamp of 0
    _parseStamp = 0;
    _reparseInProgress = false;

    _vfsInitialisedConn = GlobalFileSystem().signal_Initialised().connect(
        sigc::mem_fun(*this, &DeclarationManager::onFilesystemInitialised)
    );
}

void DeclarationManager::shutdownModule()
{
    _vfsInitialisedConn.disconnect();

    auto declLock = std::make_unique<std::lock_guard<std::recursive_mutex>>(_declarationAndCreatorLock);
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
