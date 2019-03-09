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

#include "iradiant.h"
#include "idatastream.h"
#include "ifilesystem.h"
#include "iregistry.h"
#include "igame.h"
#include "itextstream.h"

#include "string/string.h"
#include "string/join.h"
#include "string/case_conv.h"
#include "os/path.h"
#include "os/dir.h"

#include "string/split.h"
#include "debugging/ScopedDebugTimer.h"

#include "DirectoryArchive.h"
#include "DirectoryArchiveFile.h"
#include "DirectoryArchiveTextFile.h"
#include "SortedFilenames.h"
#include "ZipArchive.h"
#include "modulesystem/StaticModule.h"

namespace vfs
{

namespace
{

// Representation of an assets.lst file, containing visibility information for
// assets within a particular folder.
class AssetsList
{
    std::map<std::string, Visibility> _visibilities;

    // Convert visibility string to enum value
    static Visibility toVisibility(const std::string& input)
    {
        if (string::starts_with(input, "hid" /* 'hidden' or 'hide'*/))
        {
            return Visibility::HIDDEN;
        }
        else if (input == "normal")
        {
            return Visibility::NORMAL;
        }
        else
        {
            rWarning() << "AssetsList: failed to parse visibility '" << input
                       << "'" << std::endl;
            return Visibility::NORMAL;
        }
    }

public:

    static constexpr const char* FILENAME = "assets.lst";

    // Construct with possible ArchiveTextFile pointer containing an assets.lst
    // file to parse.
    explicit AssetsList(ArchiveTextFilePtr inputFile)
    {
        if (inputFile)
        {
            // Read lines from the file
            std::istream stream(&inputFile->getInputStream());
            while (stream.good())
            {
                std::string line;
                std::getline(stream, line);

                // Attempt to parse the line as "asset=visibility"
                std::vector<std::string> tokens;
                string::split(tokens, line, "=");

                // Parsing was a success if we have two tokens
                if (tokens.size() == 2)
                {
                    std::string filename = tokens[0];
                    Visibility v = toVisibility(tokens[1]);
                    _visibilities[filename] = v;
                }
            }
        }
    }

    // Return visibility for a given file
    Visibility getVisibility(const std::string& fileName) const
    {
        auto i = _visibilities.find(fileName);
        if (i == _visibilities.end())
        {
            return Visibility::NORMAL;
        }
        else
        {
            return i->second;
        }
    }
};

/*
 * Archive::Visitor class used in GlobalFileSystem().foreachFile().
 * It's filtering out the files matching the defined extension only.
 * The directory part is cut off the filename.
 * On top of that, this class maintains a list of visited files to avoid
 * hitting the same file twice (it might be present in more than one Archive).
 */
class FileVisitor: public Archive::Visitor
{
    // Maximum traversal depth
    std::size_t _maxDepth;

    // User-supplied functor to invoke for each file
    VirtualFileSystem::VisitorFunc _visitorFunc;

    // Optional AssetsList containing visibility information
    const AssetsList* _assetsList = nullptr;

    // Set of already-visited files to avoid visiting the same file twice
    std::set<std::string> _visitedFiles;

    // Directory to search within
    std::string _directory;

    // Extension to match
    std::string _extension;

    // The length of the directory name
    std::size_t _dirPrefixLength;

    bool _visitAll;

    std::size_t _extLength;

public:

    // Constructor. Pass "*" as extension to have it visit all files.
    FileVisitor(const VirtualFileSystem::VisitorFunc& visitorFunc,
                const std::string& dir, const std::string& ext,
                std::size_t maxDepth)
    : _maxDepth(maxDepth), _visitorFunc(visitorFunc), _directory(dir),
      _extension(ext), _dirPrefixLength(_directory.length()),
      _visitAll(_extension == "*"), _extLength(_extension.length())
    {}

    void setAssetsList(const AssetsList& list)
    {
        _assetsList = &list;
    }

    // Archive::Visitor interface

    void visitFile(const std::string& name)
    {
#ifdef OS_CASE_INSENSITIVE
        // The name should start with the directory, "def/" for instance, case-insensitively.
        assert(string::to_lower_copy(name.substr(0, _dirPrefixLength)) == _directory);
#else
        // Linux: The name should start with the directory, "def/" for instance, including case.
        assert(name.substr(0, _dirPrefixLength) == _directory);
#endif

        // Cut off the base directory prefix
        std::string subname = name.substr(_dirPrefixLength);

        // Check for matching file extension
        if (!_visitAll)
        {
            // The dot must be at the right position
            if (subname.length() <= _extLength || subname[subname.length() - _extLength - 1] != '.')
            {
                return;
            }

            // And the extension must match
            std::string ext = subname.substr(subname.length() - _extLength);

#ifdef OS_CASE_INSENSITIVE
            // Treat extensions case-insensitively in Windows
            string::to_lower(ext);
#endif

            if (ext != _extension)
            {
                return; // extension mismatch
            }
        }

        if (_visitedFiles.find(subname) != _visitedFiles.end())
        {
            return; // already visited
        }

        // Don't return assets.lst itself
        if (subname == AssetsList::FILENAME)
            return;

        // Suitable file, call the callback and add to visited file set
        vfs::Visibility vis = _assetsList ? _assetsList->getVisibility(subname)
                                          : Visibility::NORMAL;
        _visitorFunc(vfs::FileInfo{_directory, subname, vis});

        _visitedFiles.insert(subname);
    }

    bool visitDirectory(const std::string& name, std::size_t depth)
    {
        if (depth == _maxDepth)
        {
            return true;
        }

        return false;
    }
};

}

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
        os::foreachItemInDirectory(path, [&](const fs::path& file)
        {
            // Just insert the name, it will get sorted correctly.
            filenameList.insert(file.filename().string());
        });
    }
    catch (os::DirectoryNotFoundException&)
    {
        rConsole() << "[vfs] Directory '" << path << "' not found." << std::endl;
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

void Doom3FileSystem::initialise(const SearchPaths& vfsSearchPaths, const ExtensionSet& allowedExtensions)
{
    // Check if the new configuration is any different then the current one
    if (!vfsSearchPaths.empty() && vfsSearchPaths == _vfsSearchPaths && allowedExtensions == _allowedExtensions)
    {
        rMessage() << "VFS::initialise call has identical arguments as current setup, won't do anything." << std::endl;
        return;
    }

    if (!_vfsSearchPaths.empty())
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

    for (Observer* observer : _observers)
    {
        observer->onFileSystemInitialise();
    }
}

void Doom3FileSystem::shutdown()
{
    for (Observer* observer : _observers)
    {
        observer->onFileSystemShutdown();
    }

    _archives.clear();
    _directories.clear();
    _vfsSearchPaths.clear();
    _allowedExtensions.clear();
    _allowedExtensionsDir.clear();

    rMessage() << "Filesystem shut down" << std::endl;
}

void Doom3FileSystem::addObserver(Observer& observer)
{
    _observers.insert(&observer);
}

void Doom3FileSystem::removeObserver(Observer& observer)
{
    _observers.erase(&observer);
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

ArchiveFilePtr Doom3FileSystem::openFile(const std::string& filename)
{
    if (filename.find("\\") != std::string::npos)
    {
        rError() << "Filename contains backslash: " << filename << std::endl;
        return ArchiveFilePtr();
    }

    for (const ArchiveDescriptor& descriptor : _archives)
    {
        ArchiveFilePtr file = descriptor.archive->openFile(filename);

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
    std::shared_ptr<archive::DirectoryArchiveFile> file =
        std::make_shared<archive::DirectoryArchiveFile>(filename, filename);

    if (!file->failed())
    {
        return file;
    }

    return ArchiveFilePtr();
}

ArchiveTextFilePtr Doom3FileSystem::openTextFile(const std::string& filename)
{
    for (const ArchiveDescriptor& descriptor : _archives)
    {
        ArchiveTextFilePtr file = descriptor.archive->openTextFile(filename);

        if (file)
        {
            return file;
        }
    }

    return ArchiveTextFilePtr();
}

ArchiveTextFilePtr Doom3FileSystem::openTextFileInAbsolutePath(const std::string& filename)
{
    std::shared_ptr<archive::DirectoryArchiveTextFile> file =
        std::make_shared<archive::DirectoryArchiveTextFile>(filename, filename, filename);

    if (!file->failed())
    {
        return file;
    }

    return ArchiveTextFilePtr();
}

void Doom3FileSystem::forEachFile(const std::string& basedir,
    const std::string& extension,
    const VisitorFunc& visitorFunc,
    std::size_t depth)
{
    // Look for an assets.lst in the base dir
    std::string assetsLstName = basedir + AssetsList::FILENAME;
    ArchiveTextFilePtr assetsLstFile = openTextFile(assetsLstName);
    AssetsList assetsList(assetsLstFile);

    // Construct our FileVisitor filtering out the right elements
    FileVisitor fileVisitor(visitorFunc, basedir, extension, depth);
    fileVisitor.setAssetsList(assetsList);

    // Visit each Archive, applying the FileVisitor to each one (which in
    // turn calls the callback for each matching file.
    for (const ArchiveDescriptor& descriptor : _archives)
    {
        descriptor.archive->traverse(fileVisitor, basedir);
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

    tempArchive.traverse(fileVisitor, "/");
}

std::string Doom3FileSystem::findFile(const std::string& name)
{
    for (const ArchiveDescriptor& descriptor : _archives)
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
    for (const ArchiveDescriptor& descriptor : _archives)
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

const SearchPaths& Doom3FileSystem::getVfsSearchPaths()
{
    // Should not be called before the list is initialised
    if (_vfsSearchPaths.empty())
    {
        rConsole() << "Warning: VFS search paths not yet initialised." << std::endl;
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

void Doom3FileSystem::initialiseModule(const ApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called" << std::endl;
}

void Doom3FileSystem::shutdownModule()
{
    shutdown();
}

}
