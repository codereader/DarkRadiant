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

#if !defined(INCLUDED_IARCHIVE_H)
#define INCLUDED_IARCHIVE_H

/**
 * \file iarchive.h
 * Types relating to the use of ZIP archives (PK4 files) and their contents.
 * \ingroup vfs
 */

#include "ModResource.h"

#include "imodule.h"
#include <cstddef>

#include "itextstream.h"

#include <string>

class InputStream;

/**
 * A file opened in binary mode.
 * \ingroup vfs
 */
class ArchiveFile
{
public:
    /// \brief destructor
	virtual ~ArchiveFile() {}
	/// \brief Returns the size of the file data in bytes.
	virtual std::size_t size() const = 0;
	/// \brief Returns the path to this file (relative to the filesystem root)
	virtual const std::string& getName() const = 0;
	/// \brief Returns the stream associated with this file.
	/// Subsequent calls return the same stream.
	/// The stream may be read forwards until it is exhausted.
	/// The stream remains valid for the lifetime of the file.
	virtual InputStream& getInputStream() = 0;
};
typedef boost::shared_ptr<ArchiveFile> ArchiveFilePtr;

/**
 * A file opened in text mode.
 * \ingroup vfs
 */
class ArchiveTextFile : 
	public ModResource
{
public:
	/// \brief Returns the path to this file (relative to the filesystem root)
	virtual const std::string& getName() const = 0;
		
	/// \brief Returns the stream associated with this file.
	/// Subsequent calls return the same stream.
	/// The stream may be read forwards until it is exhausted.
	/// The stream remains valid for the lifetime of the file.
	virtual TextInputStream& getInputStream() = 0;
};
typedef boost::shared_ptr<ArchiveTextFile> ArchiveTextFilePtr;

class CustomArchiveVisitor;

/**
 * Representation of a ZIP archive.
 * 
 * \ingroup vfs
 */
class Archive
{
public:
	/**
	 * Visitor class for traversing files within an Archive.
	 */
	class Visitor
	{
	public:
	    virtual ~Visitor() {}
		virtual void visit(const std::string& name) = 0;
	};

	typedef CustomArchiveVisitor VisitorFunc;

	enum EMode {
		eFiles = 0x01,
		eDirectories = 0x02,
		eFilesAndDirectories = 0x03,
	};

	/// \brief destructor
	virtual ~Archive() {}

	/// \brief Returns a new object associated with the file identified by \p name, or 0 if the file cannot be opened.
	/// Name comparisons are case-insensitive.
	virtual ArchiveFilePtr openFile(const std::string& name) = 0;
	
	/// \brief Returns a new object associated with the file identified by \p name, or 0 if the file cannot be opened.
	/// Name comparisons are case-insensitive.
	virtual ArchiveTextFilePtr openTextFile(const std::string& name) = 0;
	
	/// Returns true if the file identified by \p name can be opened.
	/// Name comparisons are case-insensitive.
	virtual bool containsFile(const std::string& name) = 0;

  /// \brief Performs a depth-first traversal of the archive tree starting at \p root.
  /// Traverses the entire tree if \p root is "".
  /// When a file is encountered, calls \c visitor.file passing the file name.
  /// When a directory is encountered, calls \c visitor.directory passing the directory name.
  /// Skips the directory if \c visitor.directory returned true.
  /// Root comparisons are case-insensitive.
  /// Names are mixed-case.
  virtual void forEachFile(VisitorFunc visitor, const std::string& root) = 0;
};
typedef boost::shared_ptr<Archive> ArchivePtr;

class CustomArchiveVisitor
{
  Archive::Visitor* m_visitor;
  Archive::EMode m_mode;
  std::size_t m_depth;
public:
  CustomArchiveVisitor(Archive::Visitor& visitor, Archive::EMode mode, std::size_t depth)
    : m_visitor(&visitor), m_mode(mode), m_depth(depth)
  {
  }
  void file(const std::string& name)
  {
    if((m_mode & Archive::eFiles) != 0)
      m_visitor->visit(name);
  }
  bool directory(const std::string& name, std::size_t depth)
  {
    if ((m_mode & Archive::eDirectories) != 0) {
      m_visitor->visit(name);
    }
    if (depth == m_depth) {
      return true;
    }
    return false;
  }
};

const std::string MODULE_ARCHIVE("Archive");

/**
 * Loader module for ZIP archives.
 * 
 * \ingroup vfs
 */
class ArchiveLoader :
	public RegisterableModule
{
public:
	// greebo: Returns the opened file or NULL if failed.
	virtual ArchivePtr openArchive(const std::string& name) = 0;

    // get the supported file extension
    virtual const std::string& getExtension() = 0;
};

/**
 * Return an ArchiveLoader module for the specified filetype.
 * 
 * \ingroup vfs
 */
inline ArchiveLoader& GlobalArchive(const std::string& fileType) {
	// Cache the reference locally
	static ArchiveLoader& _archive(
		*boost::static_pointer_cast<ArchiveLoader>(
			module::GlobalModuleRegistry().getModule(MODULE_ARCHIVE + fileType) // e.g. ArchivePK4
		)
	);
	return _archive;
}

#endif
