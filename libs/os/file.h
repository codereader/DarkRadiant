#pragma once

#include "itextstream.h"
#include "fs.h"
#include "debugging/debugging.h"

/// \file
/// \brief OS file-system querying and manipulation.

#ifdef WIN32
#include <io.h> // _access()

#ifdef _MSC_VER
	// greebo: The Windows API doesn't define R_OK et al, but luckily 
	// the _access method takes the same integer/bit values as the POSIX access()
	#define F_OK 0x00
	#define W_OK 0x02
	#define R_OK 0x04
#endif

#else
#include <unistd.h> // access()
#endif

namespace os
{

namespace detail
{

	enum class FileAccess
	{
		Read = R_OK,
		Write = W_OK,
		ReadWrite = Read | Write,
		Exists = F_OK
	};

	/// \brief Returns true if the file or directory identified by \p path exists and/or may be 
	/// accessed for reading, writing or both, depending on the value of \p mode.
	inline bool checkFileAccess(const std::string& path, FileAccess mode)
	{
#ifdef WIN32
		return ::_access(path.c_str(), static_cast<int>(mode)) == 0;
#else
		return ::access(path.c_str(), static_cast<int>(mode)) == 0;
#endif
	}

}

/// \brief Returns true if the file or directory identified by \p path exists and may be opened for reading.
inline bool fileIsReadable(const std::string& path)
{
	return detail::checkFileAccess(path, detail::FileAccess::Read);
}

/// \brief Returns true if the file or directory identified by \p path exists and may be opened for reading.
inline bool fileIsReadable(const fs::path& path)
{
	return detail::checkFileAccess(path.string(), detail::FileAccess::Read);
}

/// \brief Returns true if the file or directory identified by \p path exists and may be opened for writing.
inline bool fileIsWritable(const std::string& path)
{
	return detail::checkFileAccess(path, detail::FileAccess::Write);
}

/// \brief Returns true if the file or directory identified by \p path exists and may be opened for writing.
inline bool fileIsWritable(const fs::path& path)
{
	return detail::checkFileAccess(path.string(), detail::FileAccess::Write);
}

/// \brief Returns true if the file or directory identified by \p path exists.
inline bool fileOrDirExists(const std::string& path)
{
	try
	{
		return fs::exists(path);
	}
	catch (fs::filesystem_error&)
	{
		return false;
	}
}

// Returns the file size in bytes, or static_cast<uintmax_t>(-1)
inline std::size_t getFileSize(const std::string& path)
{
	try
	{
		return static_cast<std::size_t>(fs::file_size(path));
	}
	catch (fs::filesystem_error& err)
	{
		rError() << "Error checking filesize: " << err.what() << std::endl;
		return static_cast<std::size_t>(-1);
	}
}

} // namespace
