#pragma once

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
typedef std::shared_ptr<ArchiveFile> ArchiveFilePtr;

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
typedef std::shared_ptr<ArchiveTextFile> ArchiveTextFilePtr;

/**
 * Representation of an archive in the virtual filesystem.
 * This might be a PK4/ZIP file or a regular mod directory.
 *
 * \ingroup vfs
 */
class Archive
{
public:
	class Visitor
	{
	public:
		virtual ~Visitor() {}

		// Invoked for each file in an Archive
		virtual void visitFile(const std::string& name) = 0;

		// Invoked for each directory in an Archive. Return true to skip the directory.
		virtual bool visitDirectory(const std::string& name, std::size_t depth) = 0;
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
	virtual void traverse(Visitor& visitor, const std::string& root) = 0;
};
typedef std::shared_ptr<Archive> ArchivePtr;
