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

#include "UnixPath.h"
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

// Visitor class for archives (directory or PK4)
class ArchiveVisitor: public Archive::Visitor
{
private:
    std::function<void(const std::string&)> _visitorFunc;
    Archive::EMode _mode;
    std::size_t _depth;

public:
    ArchiveVisitor(const std::function<void(const std::string&)>& func, Archive::EMode mode, std::size_t depth) :
        _visitorFunc(func),
        _mode(mode),
        _depth(depth)
    {}

    void visitFile(const std::string& name)
    {
        if ((_mode & Archive::eFiles) != 0)
        {
            _visitorFunc(name);
        }
    }

    bool visitDirectory(const std::string& name, std::size_t depth)
    {
        if ((_mode & Archive::eDirectories) != 0)
        {
            _visitorFunc(name);
        }

        if (depth == _depth)
        {
            return true;
        }

        return false;
    }
};

/*
 * Adaptor class used in GlobalFileSystem().foreachFile().
 * It's filtering out the files matching the defined extension only.
 * Passes the filename to the VisitorFunc given in its constructor.
 * The directory part is cut off the filename before it's passed to the VisitorFunc.
 * On top of that, this class maintains a list of visited files to avoid
 * hitting the same file twice (it might be present in more than one Archive).
 */
class FileVisitor
{
private:
    // The VirtualFileSystem::Visitor to call for each located file
    VirtualFileSystem::VisitorFunc _visitorFunc;

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
        const std::string& dir,
        const std::string& ext)
        : _visitorFunc(visitorFunc),
        _directory(dir),
        _extension(ext),
        _dirPrefixLength(_directory.length()),
        _visitAll(_extension == "*"),
        _extLength(_extension.length())
    {}

    // Required visit function
    void visit(const std::string& name)
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

        // Suitable file, call the callback and add to visited file set
        _visitorFunc(subname);

        _visitedFiles.insert(subname);
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
    std::string fixedFilename(os::standardPathWithSlash(filename));

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
    // Construct our FileVisitor filtering out the right elements
    FileVisitor fileVisitor(visitorFunc, basedir, extension);

    // Construct an ArchiveVisitor filtering out the files only, and watching the recursion depth
    ArchiveVisitor functor(std::bind(&FileVisitor::visit, fileVisitor, std::placeholders::_1), Archive::eFiles, depth);

    // Visit each Archive, applying the FileVisitor to each one (which in
    // turn calls the callback for each matching file.
    for (const ArchiveDescriptor& descriptor : _archives)
    {
        descriptor.archive->traverse(functor, basedir);
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
    FileVisitor fileVisitor(visitorFunc, "", extension);

    // Construct an ArchiveVisitor filtering out the files only, and watching the recursion depth
    ArchiveVisitor functor(std::bind(&FileVisitor::visit, fileVisitor, std::placeholders::_1), Archive::eFiles, depth);

    tempArchive.traverse(functor, "/");
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

// Static module instance
module::StaticModule<Doom3FileSystem> doom3FileSystemModule;

}
