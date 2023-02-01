#pragma once

/// \file
/// \brief OS directory-listing object.
#include "fs.h"
#include "itextstream.h"

#include <string>
#include <stdexcept>
#include <functional>

namespace os
{

/**
 * Exception thrown by forEachItemInDirectory to indicate that the given directory
 * does not exist.
 */
class DirectoryNotFoundException :
	public std::runtime_error
{
public:
    DirectoryNotFoundException(const std::string& what) :
		std::runtime_error(what)
    {}
};

/**
 * @brief Invoke functor for all items in a directory.
 *
 * If the path does not exist, the functor is not invoked and the function returns false.
 *
 * @param path
 * Path to directory to iterate over.
 *
 * @param functor
 * Functor to call for each item, which must expose an operator() which accepts a const
 * fs::path reference.
 *
 * @return true if the directory exists, false otherwise
 */
template<typename F>
bool forEachItemInDirectory(const std::string& path, F functor, std::nothrow_t)
{
    fs::path start(path);
    if (!fs::exists(start))
        return false;

    for (fs::directory_iterator it(start); it != fs::directory_iterator(); ++it) {
        functor(*it);
    }
    return true;
}

/**
 * @brief Invoke functor for all items in a directory.
 *
 * If the path does not exist, a DirectoryNotFoundException will be thrown.
 *
 * @sa forEachItemInDirectory(const std::string&, F, std::nothrow_t)
 */
template<typename F>
inline void forEachItemInDirectory(const std::string& path, F functor)
{
    if (!forEachItemInDirectory(path, functor, std::nothrow))
        throw DirectoryNotFoundException("forEachItemInDirectory(): invalid directory '" + path
                                         + "'");
}

/**
 * \brief
 * Create the named directory if it doesn't exist
 *
 * \return
 * true if the directory was created or already exists, false if there was an
 * error.
 */
inline bool makeDirectory(const std::string& name)
{
	try
	{
		fs::path dirPath(name);

		// Create the directory, including nonexistent parents
		if (fs::create_directories(dirPath))
		{
			// Directory has been created, set permissions
			rMessage() << "Directory " << dirPath << " created successfully." << std::endl;

#ifdef DR_USE_STD_FILESYSTEM

#if __cpp_lib_filesystem
			// C++17 standards-compliant call to std::filesystem::permissions
			// Set permissions to rwxrwxr_x
			fs::permissions(dirPath,
				fs::perms::owner_exec | fs::perms::owner_write | fs::perms::owner_read |
				fs::perms::group_exec | fs::perms::group_write | fs::perms::group_read |
				fs::perms::others_exec | fs::perms::others_read,
				fs::perm_options::add);

#else
			// pre-C++17 call to std::filesystem::permissions, only one bitmask
			// Set permissions to rwxrwxr_x
			fs::permissions(dirPath, fs::perms::add_perms |
				fs::perms::owner_exec | fs::perms::owner_write | fs::perms::owner_read |
				fs::perms::group_exec | fs::perms::group_write | fs::perms::group_read |
				fs::perms::others_exec | fs::perms::others_read);
#endif

#else
			// Boost filesystem call to filesystem::permissions
			// Set permissions to rwxrwxr_x
			fs::permissions(dirPath, fs::add_perms |
				fs::owner_exe  | fs::owner_write | fs::owner_read |
				fs::group_exe  | fs::group_write | fs::group_read |
				fs::others_exe | fs::others_read);
#endif
		}

		// Directory already exists or has been created successfully
		return true;
	}
	catch (fs::filesystem_error& ex)
	{
		rConsoleError() << "os::makeDirectory(" << name << ") failed with error "
			<< ex.what() << " (" << ex.code().value() << ")" << std::endl;

		// Directory creation failed
		return false;
	}
}

/**
 * Deletes the contents of the given directory. Returns true on success.
 */
inline bool removeDirectory(const std::string& path)
{
	try
	{
		fs::remove_all(path);
		return true;
	}
	catch (fs::filesystem_error& ex)
	{
		rConsoleError() << "os::removeDirectory(" << path << ") failed with error "
			<< ex.what() << " (" << ex.code().value() << ")" << std::endl;

		return false;
	}
}

} // namespace os
