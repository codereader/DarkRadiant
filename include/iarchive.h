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

