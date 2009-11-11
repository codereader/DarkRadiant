/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_IFILESYSTEM_H)
#define INCLUDED_IFILESYSTEM_H

/**
 * \defgroup vfs Virtual filesystem
 * \file ifilesystem.h
 * Interface types for the VFS module.
 */

#include <cstddef>
#include <string>

#include "imodule.h"
#include "generic/callbackfwd.h"

typedef Callback1<const std::string&> FileNameCallback;

class ArchiveFile;
typedef boost::shared_ptr<ArchiveFile> ArchiveFilePtr;
class ArchiveTextFile;
typedef boost::shared_ptr<ArchiveTextFile> ArchiveTextFilePtr;
class Archive;

class ModuleObserver;

const std::string MODULE_VIRTUALFILESYSTEM("VirtualFileSystem");

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
	
	/**
	 * Interface for VFS observers.
	 * 
	 * A VFS observer is automatically notified of events relating to the
	 * VFS, including startup and shutdown.
	 */ 
	class Observer {
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
	
	/// \brief Adds a root search \p path.
	/// Called before \c initialise.
	virtual void initDirectory(const std::string& path) = 0;
	
	/// \brief Initialises the filesystem.
	/// Called after all root search paths have been added.
	virtual void initialise() = 0;
	
	/// \brief Shuts down the filesystem.
	virtual void shutdown() = 0;
	
	// greebo: Adds/removes observers to/from the VFS
	virtual void addObserver(Observer& observer) = 0;
	virtual void removeObserver(Observer& observer) = 0;
	
	// Returns the number of files in the VFS matching the given filename
	virtual int getFileCount(const std::string& filename) = 0;

	/// \brief Returns the file identified by \p filename opened in binary mode, or 0 if not found.
	/// The caller must \c release() the file returned if it is not 0.
	// greebo: Note: expects the filename to be normalised (forward slashes, trailing slash).
	virtual ArchiveFilePtr openFile(const std::string& filename) = 0;
  
	/// \brief Returns the file identified by \p filename opened in text mode, or 0 if not found.
	/// The caller must \c release() the file returned if it is not 0.
	virtual ArchiveTextFilePtr openTextFile(const std::string& filename) = 0;

  /// \brief Opens the file identified by \p filename and reads it into \p buffer, or sets *\p buffer to 0 if not found.
  /// Returns the size of the buffer allocated, or undefined value if *\p buffer is 0;
  /// The caller must free the allocated buffer by calling \c freeFile
  /// \deprecated Deprecated - use \c openFile.
	virtual std::size_t loadFile(const std::string& filename, void **buffer) = 0;
  /// \brief Frees the buffer returned by \c loadFile.
  /// \deprecated Deprecated.
  virtual void freeFile(void *p) = 0;

  /// \brief Calls \p callback for each file under \p basedir matching \p extension.
  /// Use "*" as \p extension to match all file extensions.
  virtual void forEachFile(const std::string& basedir, const std::string& extension, const FileNameCallback& callback, std::size_t depth = 1) = 0;

	/// \brief Returns the absolute filename for a relative \p name, or "" if not found.
	virtual std::string findFile(const std::string& name) = 0;

	/// \brief Returns the filesystem root for an absolute \p name, or "" if not found.
	/// This can be used to convert an absolute name to a relative name.
	virtual std::string findRoot(const std::string& name) = 0;
};

inline VirtualFileSystem& GlobalFileSystem() {
	// Cache the reference locally
	static VirtualFileSystem& _vfs(
		*boost::static_pointer_cast<VirtualFileSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_VIRTUALFILESYSTEM)
		)
	);
	return _vfs;
}

#endif
