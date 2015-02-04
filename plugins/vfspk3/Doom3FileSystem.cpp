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
#include "FileVisitor.h"

#include <stdio.h>
#include <stdlib.h>

#include "iradiant.h"
#include "idatastream.h"
#include "ifilesystem.h"
#include "iregistry.h"
#include "igame.h"

#include "string/string.h"
#include "os/path.h"
#include "os/dir.h"
#include "archivelib.h"
#include "moduleobservers.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "UnixPath.h"
#include "DirectoryArchive.h"
#include "SortedFilenames.h"

Doom3FileSystem::Doom3FileSystem() :
    _numDirectories(0)
{}

void Doom3FileSystem::initDirectory(const std::string& inputPath)
{
    if (_numDirectories == (VFS_MAXDIRS-1)) {
        return;
    }

    // greebo: Normalise path: Replace backslashes and ensure trailing slash
    _directories[_numDirectories] = os::standardPathWithSlash(inputPath);

    // Shortcut
    const std::string& path = _directories[_numDirectories];

    _numDirectories++;

    {
        ArchiveDescriptor entry;
        entry.name = path;
        entry.archive = DirectoryArchivePtr(new DirectoryArchive(path));
        entry.is_pakfile = false;
        _archives.push_back(entry);
    }

    // Instantiate a new sorting container for the filenames
    SortedFilenames filenameList;

    // Traverse the directory using the filename list as functor
    try
    {
		os::foreachItemInDirectory(path, [&] (const boost::filesystem::path& file)
		{
			// Just insert the name, it will get sorted correctly.
			filenameList.insert(file.filename().string());
		});
    }
    catch (os::DirectoryNotFoundException&)
    {
        rConsole() << "[vfs] Directory '" << path << "' not found."
                  << std::endl;
    }

    if (filenameList.empty())
    {
        return; // nothing found
    }

    rMessage() << "[vfs] searched directory: " << path << std::endl;

    // Get the ArchiveLoader and try to load each file
    ArchiveLoader& archiveModule = GlobalArchive("PK4");

    // add the entries to the vfs
    for (SortedFilenames::iterator i = filenameList.begin(); i != filenameList.end(); ++i) {
        // Assemble the filename and try to load the archive
        initPakFile(archiveModule, path + *i);
    }
}

void Doom3FileSystem::initialise()
{
    rMessage() << "filesystem initialised" << std::endl;

    std::string extensions = GlobalGameManager().currentGame()->getKeyValue("archivetypes");
    boost::algorithm::split(_allowedExtensions, extensions, boost::algorithm::is_any_of(" "));

    // Build list of dir extensions, e.g. pk4 -> pk4dir
    for (std::set<std::string>::const_iterator i = _allowedExtensions.begin();
         i != _allowedExtensions.end();
         ++i)
    {
        std::string extDir = *i + "dir";
        _allowedExtensionsDir.insert(extDir);
    }

    // Get the VFS search paths from the game manager
    const game::IGameManager::PathList& paths =
        GlobalGameManager().getVFSSearchPaths();

    // Initialise the paths, in the given order
    for (game::IGameManager::PathList::const_iterator i = paths.begin();
         i != paths.end(); i++)
    {
        initDirectory(*i);
    }

    for (ObserverList::iterator i = _observers.begin(); i != _observers.end(); ++i)
    {
        (*i)->onFileSystemInitialise();
    }
}

void Doom3FileSystem::shutdown() {
    for (ObserverList::iterator i = _observers.begin(); i != _observers.end(); ++i)
    {
        (*i)->onFileSystemShutdown();
    }

    rMessage() << "filesystem shutdown" << std::endl;

    _archives.clear();
    _numDirectories = 0;
}

void Doom3FileSystem::addObserver(Observer& observer) {
    _observers.insert(&observer);
}

void Doom3FileSystem::removeObserver(Observer& observer) {
    _observers.erase(&observer);
}

int Doom3FileSystem::getFileCount(const std::string& filename) {
    int count = 0;
    std::string fixedFilename(os::standardPathWithSlash(filename));

    for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
        if (i->archive->containsFile(fixedFilename.c_str())) {
            ++count;
        }
    }

    return count;
}

ArchiveFilePtr Doom3FileSystem::openFile(const std::string& filename) {
    if (filename.find("\\") != std::string::npos) {
        rError() << "Filename contains backslash: " << filename << std::endl;
        return ArchiveFilePtr();
    }

    for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
        ArchiveFilePtr file = i->archive->openFile(filename);
        if (file != NULL) {
            return file;
        }
    }

    // not found
    return ArchiveFilePtr();
}

ArchiveFilePtr Doom3FileSystem::openFileInAbsolutePath(const std::string& filename)
{
    DirectoryArchiveFilePtr file(new DirectoryArchiveFile(filename, filename));

    if (!file->failed())
    {
        return file;
    }

    return ArchiveFilePtr();
}

ArchiveTextFilePtr Doom3FileSystem::openTextFile(const std::string& filename) {
    for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
        ArchiveTextFilePtr file = i->archive->openTextFile(filename);
        if (file != NULL) {
            return file;
        }
    }

    return ArchiveTextFilePtr();
}

ArchiveTextFilePtr Doom3FileSystem::openTextFileInAbsolutePath(const std::string& filename)
{
    DirectoryArchiveTextFilePtr file(new DirectoryArchiveTextFile(filename, filename, filename));

    if (!file->failed())
    {
        return file;
    }

    return ArchiveTextFilePtr();
}

std::size_t Doom3FileSystem::loadFile(const std::string& filename, void **buffer) {
    std::string fixedFilename(os::standardPathWithSlash(filename));

    ArchiveFilePtr file = openFile(fixedFilename);

    if (file != NULL) {
        // Allocate one byte more for the trailing zero
        *buffer = malloc(file->size()+1);

        // we need to end the buffer with a 0
        ((char*) (*buffer))[file->size()] = 0;

        std::size_t length = file->getInputStream().read(
            reinterpret_cast<InputStream::byte_type*>(*buffer),
            file->size()
        );

        return length;
    }

    *buffer = NULL;
    return 0;
}

void Doom3FileSystem::freeFile(void *p) {
    free(p);
}

void Doom3FileSystem::forEachFile(const std::string& basedir,
                                  const std::string& extension,
                                  const VisitorFunc& visitorFunc,
                                  std::size_t depth)
{
    // Set of visited files, to avoid name conflicts
    std::set<std::string> visitedFiles;

    // Wrap around the passed visitor
    FileVisitor visitor2(visitorFunc, basedir, extension, visitedFiles);

    // Visit each Archive, applying the FileVisitor to each one (which in
    // turn calls the callback for each matching file.
    for (ArchiveList::iterator i = _archives.begin();
         i != _archives.end();
         ++i)
    {
        i->archive->forEachFile(Archive::VisitorFunc(visitor2, Archive::eFiles, depth), basedir);
    }
}

void Doom3FileSystem::forEachFileInAbsolutePath(const std::string& path,
                                                const std::string& extension,
                                                const VisitorFunc& visitorFunc,
                                                std::size_t depth)
{
    std::set<std::string> visitedFiles;

    // Construct a temporary DirectoryArchive from the given path
    DirectoryArchive tempArchive(os::standardPathWithSlash(path));

    // Wrap around the passed visitor
    FileVisitor visitor2(visitorFunc, "", extension, visitedFiles);

    tempArchive.forEachFile(Archive::VisitorFunc(visitor2, Archive::eFiles, depth), "/");
}

std::string Doom3FileSystem::findFile(const std::string& name) {
    for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
        if (!i->is_pakfile && i->archive->containsFile(name.c_str())) {
            return i->name;
        }
    }

    return "";
}

std::string Doom3FileSystem::findRoot(const std::string& name) {
    for (ArchiveList::iterator i = _archives.begin(); i != _archives.end(); ++i) {
        if (!i->is_pakfile && path_equal_n(name.c_str(), i->name.c_str(), i->name.size())) {
            return i->name;
        }
    }

    return "";
}

void Doom3FileSystem::initPakFile(ArchiveLoader& archiveModule, const std::string& filename)
{
    std::string fileExt(os::getExtension(filename));
    boost::to_lower(fileExt);

    if (_allowedExtensions.find(fileExt) != _allowedExtensions.end())
    {
        // Matched extension for archive (e.g. "pk3", "pk4")
        ArchiveDescriptor entry;

        entry.name = filename;
        entry.archive = archiveModule.openArchive(filename);
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
        entry.archive = DirectoryArchivePtr(new DirectoryArchive(path));
        entry.is_pakfile = false;
        _archives.push_back(entry);

        rMessage() << "[vfs] pak dir:  " << path << std::endl;
    }
}

// RegisterableModule implementation
const std::string& Doom3FileSystem::getName() const {
    static std::string _name(MODULE_VIRTUALFILESYSTEM);
    return _name;
}

const StringSet& Doom3FileSystem::getDependencies() const {
    static StringSet _dependencies;

    if (_dependencies.empty()) {
        _dependencies.insert("ArchivePK4");
        _dependencies.insert(MODULE_XMLREGISTRY);
        _dependencies.insert(MODULE_GAMEMANAGER);
    }

    return _dependencies;
}

void Doom3FileSystem::initialiseModule(const ApplicationContext& ctx)
{
    rMessage() << "VFS::initialiseModule called" << std::endl;

    initialise();
}
