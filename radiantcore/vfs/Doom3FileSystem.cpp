//
// Rules:
//
// - Directories should be searched in the following order: ~/.q3a/baseq3,
//   install dir (/usr/local/games/quake3/baseq3) and cd_path (/mnt/cdrom/baseq3).
//
// - Pak files are searched first inside the directories.
// - Case insensitive.
// - Unix-style slashes (/) (windows is backwards .. everyone knows that)
//
// Leonardo Zide (leo@lokigames.com)
//

#include "Doom3FileSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale>

#include "iradiant.h"
#include "idatastream.h"
#include "ifilesystem.h"
#include "iregistry.h"
#include "igame.h"
#include "itextstream.h"

#include "string/string.h"
#include "string/join.h"
#include "string/case_conv.h"
#include "string/encoding.h"
#include "os/path.h"
#include "os/dir.h"
#include "os/file.h"

#include "string/split.h"
#include "debugging/ScopedDebugTimer.h"

#include "DirectoryArchive.h"
#include "DirectoryArchiveFile.h"
#include "DirectoryArchiveTextFile.h"
#include "SortedFilenames.h"
#include "ZipArchive.h"
#include "module/StaticModule.h"
#include "FileVisitor.h"

namespace vfs
{

void Doom3FileSystem::initDirectory(const std::string& inputPath)
{
    // greebo: Normalise path: Replace backslashes and ensure trailing slash
    _directories.push_back(os::standardPathWithSlash(inputPath));

    // Shortcut
    const std::string& path = _directories.back();

    {
        ArchiveDescriptor entry;
        entry.name = path;
        entry.archive = std::make_shared<DirectoryArchive>(path);
        entry.is_pakfile = false;

        _archives.push_back(entry);
    }

    // Instantiate a new sorting container for the filenames
    SortedFilenames filenameList;

    // Traverse the directory using the filename list as functor
    try
    {
        os::forEachItemInDirectory(path, [&](const fs::path& file)
        {
            try
            {
                // Just insert the name, it will get sorted correctly.
                filenameList.insert(file.filename().string());
            }
            catch (std::system_error& ex)
            {
                rWarning() << "[vfs] Skipping file " << string::unicode_to_utf8(file.filename().wstring()) <<
                    " - possibly unsupported characters in filename? " << 
                    "(Exception: " << ex.what() << ")" << std::endl;
            }
        });
    }
    catch (os::DirectoryNotFoundException&)
    {
        rError() << "[vfs] Directory '" << path << "' not found." << std::endl;
    }

    if (filenameList.empty())
    {
        return; // nothing found
    }

    rMessage() << "[vfs] Searched directory: " << path << std::endl;

    // add the entries to the vfs
    for (const std::string& filename : filenameList)
    {
        // Assemble the filename and try to load the archive
        initPakFile(path + filename);
    }
}

void Doom3FileSystem::initialise(const SearchPaths& vfsSearchPaths, const std::set<std::string>& allowedExtensions)
{
    // Check if the new configuration is any different then the current one
    if (!vfsSearchPaths.empty() && vfsSearchPaths == _vfsSearchPaths && allowedExtensions == _allowedExtensions)
    {
        rMessage() << "VFS::initialise call has identical arguments as current setup, won't do anything." << std::endl;
        return;
    }

    if (isInitialised())
    {
        // We've been initialised with some paths already, shutdown first
        shutdown();
    }

    _vfsSearchPaths = vfsSearchPaths;
    _allowedExtensions = allowedExtensions;

    rMessage() << "Initialising filesystem using " << _vfsSearchPaths.size() << " paths " << std::endl;
    rMessage() << "VFS Search Path priority is: \n- " << string::join(_vfsSearchPaths, "\n- ") << std::endl;

    rMessage() << "Allowed PK4 Archive File Extensions: " << string::join(_allowedExtensions, ", ") << std::endl;

    // Build list of dir extensions, e.g. pk4 -> pk4dir
    for (const std::string& allowedExtension : _allowedExtensions)
    {
        _allowedExtensionsDir.insert(allowedExtension + "dir");
    }

    // Initialise the paths, in the given order
    for (const std::string& path : _vfsSearchPaths)
    {
        initDirectory(path);
    }

    signal_Initialised().emit();
}

bool Doom3FileSystem::isInitialised() const
{
    return !_vfsSearchPaths.empty();
}

void Doom3FileSystem::shutdown()
{
    _archives.clear();
    _directories.clear();
    _vfsSearchPaths.clear();
    _allowedExtensions.clear();
    _allowedExtensionsDir.clear();

    rMessage() << "Filesystem shut down" << std::endl;
}

const std::set<std::string>& Doom3FileSystem::getArchiveExtensions() const
{
    return _allowedExtensions;
}

int Doom3FileSystem::getFileCount(const std::string& filename)
{
    int count = 0;
    std::string fixedFilename(os::standardPath(filename));

    for (const ArchiveDescriptor& descriptor : _archives)
    {
        if (descriptor.archive->containsFile(fixedFilename))
        {
            ++count;
        }
    }

    return count;
}

FileInfo Doom3FileSystem::getFileInfo(const std::string& vfsRelativePath)
{
    for (const auto& descriptor : _archives)
    {
        if (!descriptor.archive->containsFile(vfsRelativePath))
        {
            continue;
        }

        // Determine the visibility of this file
        auto topLevelDir = os::getToplevelDirectory(vfsRelativePath);

        auto visibility = Visibility::NORMAL;
        auto assetsList = findAssetsList(topLevelDir);

        if (assetsList)
        {
            // Information in the assets list file are relative to the top-level dir
            auto relativePath = os::getRelativePath(vfsRelativePath, topLevelDir);
            visibility = assetsList->getVisibility(relativePath);
        }

        return FileInfo("", vfsRelativePath, visibility, *descriptor.archive);
    }

    return FileInfo();
}

ArchiveFilePtr Doom3FileSystem::openFile(const std::string& filename)
{
    if (filename.find("\\") != std::string::npos)
    {
        rError() << "Filename contains backslash: " << filename << std::endl;
        return ArchiveFilePtr();
    }

    for (const auto& descriptor : _archives)
    {
        auto file = descriptor.archive->openFile(filename);

        if (file)
        {
            return file;
        }
    }

    // not found
    return ArchiveFilePtr();
}

ArchiveFilePtr Doom3FileSystem::openFileInAbsolutePath(const std::string& filename)
{
    auto file = std::make_shared<archive::DirectoryArchiveFile>(filename, filename);

    if (!file->failed())
    {
        return file;
    }

    return ArchiveFilePtr();
}

ArchiveTextFilePtr Doom3FileSystem::openTextFile(const std::string& filename)
{
    for (const auto& descriptor : _archives)
    {
        auto file = descriptor.archive->openTextFile(filename);

        if (file)
        {
            return file;
        }
    }

    return ArchiveTextFilePtr();
}

ArchiveTextFilePtr Doom3FileSystem::openTextFileInAbsolutePath(const std::string& filename)
{
    auto file = std::make_shared<archive::DirectoryArchiveTextFile>(filename, filename, filename);

    if (!file->failed())
    {
        return file;
    }

    return ArchiveTextFilePtr();
}

IArchive::Ptr Doom3FileSystem::openArchiveInAbsolutePath(const std::string& pathToArchive)
{
    if (!os::fileIsReadable(pathToArchive))
    {
        rWarning() << "Requested file is not readable: " << pathToArchive << std::endl;
        return IArchive::Ptr();
    }

    return std::make_shared<archive::ZipArchive>(pathToArchive);
}

std::shared_ptr<AssetsList> Doom3FileSystem::findAssetsList(const std::string& topLevelDir)
{
    // Look for an assets.lst in the top-level dir (can be an empty empty)
    std::string assetsLstName = topLevelDir + AssetsList::FILENAME;

    auto assetsLstFile = openTextFile(assetsLstName);
    return std::make_shared<AssetsList>(assetsLstFile);
}

void Doom3FileSystem::forEachFile(const std::string& basedir,
                                  const std::string& extension,
                                  const VisitorFunc& visitorFunc,
                                  std::size_t depth)
{
    std::string dirWithSlash = os::standardPathWithSlash(basedir);

    // Look for an assets.lst in the base dir
    auto assetsList = findAssetsList(dirWithSlash);

    // Construct our FileVisitor filtering out the right elements
    FileVisitor fileVisitor(visitorFunc, dirWithSlash, extension, depth);
    fileVisitor.setAssetsList(*assetsList);

    // Visit each Archive, applying the FileVisitor to each one (which in
    // turn calls the callback for each matching file.
    for (const auto& descriptor : _archives)
    {
        descriptor.archive->traverse(fileVisitor, dirWithSlash);
    }
}

void Doom3FileSystem::forEachFileInAbsolutePath(const std::string& path,
    const std::string& extension,
    const VisitorFunc& visitorFunc,
    std::size_t depth)
{
    // Construct a temporary DirectoryArchive from the given path
    DirectoryArchive tempArchive(os::standardPathWithSlash(path));

    // Construct our FileVisitor filtering out the right elements
    FileVisitor fileVisitor(visitorFunc, "", extension, depth);

    tempArchive.traverse(fileVisitor, "");
}

void Doom3FileSystem::forEachFileInArchive(const std::string& absoluteArchivePath,
    const std::string& extension,
    const VisitorFunc& visitorFunc,
    std::size_t depth)
{
    // Construct a temporary ZipArchive from the given path
    archive::ZipArchive tempArchive(absoluteArchivePath);

    // Construct our FileVisitor filtering out the right elements
    FileVisitor fileVisitor(visitorFunc, "", extension, depth);

    tempArchive.traverse(fileVisitor, "");
}

std::string Doom3FileSystem::findFile(const std::string& name)
{
    for (const auto& descriptor : _archives)
    {
        if (!descriptor.is_pakfile && descriptor.archive->containsFile(name))
        {
            return descriptor.name;
        }
    }

    return std::string();
}

std::string Doom3FileSystem::findRoot(const std::string& name)
{
    for (const auto& descriptor : _archives)
    {
        if (!descriptor.is_pakfile && path_equal_n(name.c_str(), descriptor.name.c_str(), descriptor.name.size()))
        {
            return descriptor.name;
        }
    }

    return std::string();
}

void Doom3FileSystem::initPakFile(const std::string& filename)
{
    std::string fileExt = string::to_lower_copy(os::getExtension(filename));

    if (_allowedExtensions.find(fileExt) != _allowedExtensions.end())
    {
        // Matched extension for archive (e.g. "pk3", "pk4")
        ArchiveDescriptor entry;

        entry.name = filename;
        entry.archive = std::make_shared<archive::ZipArchive>(filename);
        entry.is_pakfile = true;
        _archives.push_back(entry);

        rMessage() << "[vfs] pak file: " << filename << std::endl;
    }
    else if (_allowedExtensionsDir.find(fileExt) != _allowedExtensionsDir.end())
    {
        // Matched extension for archive dir (e.g. "pk3dir", "pk4dir")
        ArchiveDescriptor entry;

        std::string path = os::standardPathWithSlash(filename);
        entry.name = path;
        entry.archive = std::make_shared<DirectoryArchive>(path);
        entry.is_pakfile = false;
        _archives.push_back(entry);

        rMessage() << "[vfs] pak dir:  " << path << std::endl;
    }
}

sigc::signal<void>& Doom3FileSystem::signal_Initialised()
{
    return _sigInitialised;
}

const SearchPaths& Doom3FileSystem::getVfsSearchPaths()
{
    // Should not be called before the list is initialised
    if (_vfsSearchPaths.empty())
    {
        rWarning() << "Warning: VFS search paths not yet initialised." << std::endl;
    }

    return _vfsSearchPaths;
}

// RegisterableModule implementation
const std::string& Doom3FileSystem::getName() const
{
    static std::string _name(MODULE_VIRTUALFILESYSTEM);
    return _name;
}

const StringSet& Doom3FileSystem::getDependencies() const
{
    static StringSet _dependencies;
    return _dependencies;
}

void Doom3FileSystem::initialiseModule(const IApplicationContext& ctx)
{
}

void Doom3FileSystem::shutdownModule()
{
    shutdown();
}

// Static module instance
module::StaticModuleRegistration<Doom3FileSystem> fileSystemModule;

}
