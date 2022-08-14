#pragma once

/**
 * \defgroup vfs Virtual filesystem
 * \file ifilesystem.h
 * Interface types for the VFS module.
 */

#include <cstddef>
#include <string>
#include <list>
#include <set>
#include <functional>
#include <algorithm>
#include <sigc++/signal.h>

#include "imodule.h"
#include "iarchive.h"

namespace vfs
{

// Extension of std::list to check for existing paths before inserting new ones
class SearchPaths :
	public std::list<std::string>
{
public:
	bool insertIfNotExists(const std::string& path)
	{
		if (std::find(begin(), end(), path) != end())
		{
			return false;
		}

		push_back(path);
		return true;
	}
};

/// Visibility of an asset in the mod installation
enum class Visibility
{
    /// Standard visibility, shown in all relevant areas
    NORMAL,

    /// Hidden from selectors, but rendered as normal in the map itself
    HIDDEN
};

inline std::ostream& operator<< (std::ostream& s, const Visibility& v)
{
    if (v == Visibility::HIDDEN)
        return s << "Visibility::HIDDEN";
    else if (v == Visibility::NORMAL)
        return s << "Visibility::NORMAL";
    else
        return s << "Visibility(invalid)";
}

/// Metadata about a file in the virtual filesystem
class FileInfo
{
private:
    // Info provider to load additional info on demand, used by e.g. getSize()
    IArchiveFileInfoProvider* _infoProvider;
public:
    FileInfo() :
        FileInfo(std::string(), std::string(), Visibility::HIDDEN)
    {}

    FileInfo(const std::string& topDir_, const std::string& name_, Visibility visibility_) :
        _infoProvider(nullptr),
        topDir(topDir_),
        name(name_),
        visibility(visibility_)
    {}

    FileInfo(const std::string& topDir_, const std::string& name_, 
        Visibility visibility_, IArchiveFileInfoProvider& infoProvider) :
        FileInfo(topDir_, name_, visibility_)
    {
        _infoProvider = &infoProvider;
    }

    FileInfo(const FileInfo& other) = default;
    FileInfo(FileInfo&& other) = default;
    FileInfo& operator=(const FileInfo& other) = default;
    FileInfo& operator=(FileInfo&& other) = default;

    /// Top-level directory (if any), e.g. "def" or "models"
    std::string topDir;

    /// Name of the file, including intermediate directories under the topDir
    std::string name;

    /// Visibility of the file
    Visibility visibility = Visibility::NORMAL;

    bool isEmpty() const
    {
        return name.empty();
    }

    /// Return the full mod-relative path, including the containing directory
    std::string fullPath() const
    {
        if (topDir.empty())
            return name;
        else
            return topDir + (topDir.back() == '/' ? "" : "/") + name;
    }

    // See IArchiveFileInfoProvider::getFileSize
    std::size_t getSize() const
    {
        return _infoProvider ? _infoProvider->getFileSize(fullPath()) : 0;
    }

    // See IArchiveFileInfoProvider::getIsPhysicalFile
    bool getIsPhysicalFile() const
    {
        return _infoProvider ? _infoProvider->getIsPhysical(fullPath()) : false;
    }

    // See IArchiveFileInfoProvider::getArchivePath
    std::string getArchivePath() const
    {
        return _infoProvider ? _infoProvider->getArchivePath(fullPath()) : "";
    }

    /// Equality comparison with another FileInfo
    bool operator== (const FileInfo& rhs) const
    {
        return topDir == rhs.topDir && name == rhs.name
            && visibility == rhs.visibility;
    }
};

/**
 * Main interface for the virtual filesystem.
 *
 * The virtual filesystem provides a unified view of the contents of Doom 3's
 * base and mod subdirectories, including the contents of PK4 files. Assets can
 * be retrieved using a single unique path, without needing to know whereabouts
 * in the physical filesystem the asset is located.
 *
 * \ingroup vfs
 */
class VirtualFileSystem :
	public RegisterableModule
{
public:
	virtual ~VirtualFileSystem() {}

    // Functor taking the filename and visibility as argument. The filename is
    // relative to the base path passed to the GlobalFileSystem().foreach*()
    // method.
	typedef std::function<void(const FileInfo&)> VisitorFunc;

	// Initialises the filesystem using the given search order.
	virtual void initialise(const SearchPaths& vfsSearchPaths, const std::set<std::string>& allowedArchiveExtensions) = 0;

    // Returns true if the filesystem has already been initialised
    virtual bool isInitialised() const = 0;

	/// \brief Shuts down the filesystem.
	virtual void shutdown() = 0;

    // Returns the extension set this VFS instance has been initialised with
    virtual const std::set<std::string>& getArchiveExtensions() const = 0;

    // A signal that is emitted when the VFS has been initialised, i.e. the paths have been
    // set, the archives/directories are known and can be traversed
    virtual sigc::signal<void>& signal_Initialised() = 0;

	// Returns the number of files in the VFS matching the given filename
	virtual int getFileCount(const std::string& filename) = 0;

	/// \brief Returns the file identified by \p filename opened in binary mode, or 0 if not found.
	// greebo: Note: expects the filename to be normalised (forward slashes, trailing slash).
	virtual ArchiveFilePtr openFile(const std::string& filename) = 0;

	/// \brief Returns the file identified by \p filename opened in binary mode, or 0 if not found.
	// This is a variant of openFile taking an absolute path as argument.
	virtual ArchiveFilePtr openFileInAbsolutePath(const std::string& filename) = 0;

	/// \brief Returns the file identified by \p filename opened in text mode, or 0 if not found.
	virtual ArchiveTextFilePtr openTextFile(const std::string& filename) = 0;

	/// \brief Returns the file identified by \p filename opened in text mode, or NULL if not found.
	/// This is a variant of openTextFile taking an absolute path as argument.
	virtual ArchiveTextFilePtr openTextFileInAbsolutePath(const std::string& filename) = 0;

    // Opens an independent archive located in the given physical path.
    // (This archive can be located somewhere outside the current VFS hierarchy.)
    // Loading this archive won't have any effect on the VFS setup, it is opened stand-alone.
    virtual IArchive::Ptr openArchiveInAbsolutePath(const std::string& pathToArchive) = 0;

	/// \brief Calls the visitor function for each file under \p basedir matching \p extension.
	/// Use "*" as \p extension to match all file extensions.
	virtual void forEachFile(const std::string& basedir,
		const std::string& extension,
		const VisitorFunc& visitorFunc,
		std::size_t depth = 1) = 0;

	// Similar to forEachFile, this routine traverses an absolute path
	// searching for files matching a certain extension and invoking
	// the given visitor functor on each occurrence.
	virtual void forEachFileInAbsolutePath(const std::string& path,
		const std::string& extension,
		const VisitorFunc& visitorFunc,
		std::size_t depth = 1) = 0;

    // Similar to forEachFile, this routine traverses an archive in the given path
    // searching for files matching a certain extension and invoking
    // the given visitor functor on each occurrence.
    virtual void forEachFileInArchive(const std::string& absoluteArchivePath,
        const std::string& extension,
        const VisitorFunc& visitorFunc,
        std::size_t depth = 1) = 0;

	/// \brief Returns the absolute filename for a relative \p name, or "" if not found.
	virtual std::string findFile(const std::string& name) = 0;

	/// \brief Returns the filesystem root for an absolute \p name, or "" if not found.
	/// This can be used to convert an absolute name to a relative name.
	virtual std::string findRoot(const std::string& name) = 0;

	// Returns the list of registered VFS paths, ordered by search priority
	virtual const SearchPaths& getVfsSearchPaths() = 0;

    // Gets the file info structure for the given VFS file.
    // The info structure will be empty if the file was not located in the current VFS tree
    virtual vfs::FileInfo getFileInfo(const std::string& vfsRelativePath) = 0;
};

}

constexpr const char* const MODULE_VIRTUALFILESYSTEM("VirtualFileSystem");

inline vfs::VirtualFileSystem& GlobalFileSystem()
{
	static module::InstanceReference<vfs::VirtualFileSystem> _reference(MODULE_VIRTUALFILESYSTEM);
	return _reference;
}
