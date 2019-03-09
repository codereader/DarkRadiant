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

#include "imodule.h"

class ArchiveFile;
typedef std::shared_ptr<ArchiveFile> ArchiveFilePtr;
class ArchiveTextFile;
typedef std::shared_ptr<ArchiveTextFile> ArchiveTextFilePtr;

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
struct FileInfo
{
    /// Top-level directory (if any), e.g. "def" or "models"
    std::string topDir;

    /// Name of the file, including intermediate directories under the topDir
    std::string name;

    /// Visibility of the file
    Visibility visibility = Visibility::NORMAL;

    /// Return the full mod-relative path, including the containing directory
    std::string fullPath() const
    {
        if (topDir.empty())
            return name;
        else
            return topDir + (topDir.back() == '/' ? "" : "/") + name;
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

	/**
	 * Interface for VFS observers.
	 *
	 * A VFS observer is automatically notified of events relating to the
	 * VFS, including startup and shutdown.
	 */
	class Observer
	{
	public:
		virtual ~Observer() {}

		/**
		 * Notification of VFS initialisation.
		 *
		 * This method is invoked for all VFS observers when the VFS is
		 * initialised. An empty default implementation is provided.
		 */
		virtual void onFileSystemInitialise() {}

		/**
		 * Notification of VFS shutdown.
		 *
		 * This method is invoked for all VFS observers when the VFS is shut
		 * down. An empty default implementation is provided.
		 */
		virtual void onFileSystemShutdown() {}
	};

	typedef std::set<std::string> ExtensionSet;

	// Initialises the filesystem using the given search order.
	virtual void initialise(const SearchPaths& vfsSearchPaths, const ExtensionSet& allowedArchiveExtensions) = 0;

	/// \brief Shuts down the filesystem.
	virtual void shutdown() = 0;

	// greebo: Adds/removes observers to/from the VFS
	virtual void addObserver(Observer& observer) = 0;
	virtual void removeObserver(Observer& observer) = 0;

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

	/// \brief Calls the visitor function for each file under \p basedir matching \p extension.
	/// Use "*" as \p extension to match all file extensions.
	virtual void forEachFile(const std::string& basedir,
		const std::string& extension,
		const VisitorFunc& visitorFunc,
		std::size_t depth = 1) = 0;

	// Similar to forEachFile, this routine traverses an absolute path
	// searching for files matching a certain extension and invoking
	// the givne visitor functor on each occurrence.
	virtual void forEachFileInAbsolutePath(const std::string& path,
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
};

}

const char* const MODULE_VIRTUALFILESYSTEM("VirtualFileSystem");

inline vfs::VirtualFileSystem& GlobalFileSystem()
{
	// Cache the reference locally
	static vfs::VirtualFileSystem& _vfs(
		*std::static_pointer_cast<vfs::VirtualFileSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_VIRTUALFILESYSTEM)
		)
	);
	return _vfs;
}
