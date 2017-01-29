#pragma once

/// \file
/// \brief OS file-system querying and manipulation.

#if defined( WIN32 )
#define S_ISDIR(mode) (mode & _S_IFDIR)
#include <io.h> // _access()

#if defined(_MSC_VER)
// greebo: These are needed in VC++ 2005 Express + Platform SDK
#define F_OK 0x00
#define W_OK 0x02
#define R_OK 0x04
#endif

#define access(path, mode) _access(path, mode)
#else
#include <unistd.h> // access()
#endif

#include <stdio.h> // rename(), remove()
#include <sys/stat.h> // stat()
#include <sys/types.h> // this is included by stat.h on win32
#include <cstddef>
#include <ctime>

#include "itextstream.h"
#include <boost/filesystem/operations.hpp>
#include "debugging/debugging.h"

namespace FileAccess
{
  enum Mode
  {
    Read = R_OK,
    Write = W_OK,
    ReadWrite = Read | Write,
    Exists = F_OK
  };
}

/// \brief Returns true if the file or directory identified by \p path exists and/or may be accessed for reading, writing or both, depending on the value of \p mode.
inline bool file_accessible(const char* path, FileAccess::Mode mode)
{
  ASSERT_MESSAGE(path != 0, "file_accessible: invalid path");
  return access(path, static_cast<int>(mode)) == 0;
}

/// \brief Returns true if the file or directory identified by \p path exists and may be opened for reading.
inline bool file_readable(const char* path)
{
  return file_accessible(path, FileAccess::Read);
}

/// \brief Returns true if the file or directory identified by \p path exists and may be opened for writing.
inline bool file_writeable(const char* path)
{
  return file_accessible(path, FileAccess::Write);
}

namespace os
{

/// \brief Returns true if the file or directory identified by \p path exists.
inline bool fileOrDirExists(const std::string& path)
{
	try
	{
		return boost::filesystem::exists(path);
	}
	catch (boost::filesystem::filesystem_error&)
	{
		return false;
	}
}

// Returns the file size in bytes, or static_cast<uintmax_t>(-1)
inline std::size_t getFileSize(const std::string& path)
{
	try
	{
		return static_cast<std::size_t>(boost::filesystem::file_size(path));
	}
	catch (boost::filesystem::filesystem_error& err)
	{
		rError() << "Error checking filesize: " << err.what() << std::endl;
		return static_cast<std::size_t>(-1);
	}
}

} // namespace
