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
 * Exception thrown by foreachItemInDirectory to indicate that the given directory
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

// Invoke functor for all items in a directory
inline void foreachItemInDirectory(const std::string& path, const std::function<void(const fs::path&)>& functor)
{
	fs::path start(path);

	if (!fs::exists(start))
	{
		throw DirectoryNotFoundException("foreachItemInDirectory(): invalid directory '" + path + "'");
	}

	for (fs::directory_iterator it(start); it != fs::directory_iterator(); ++it)
	{
		functor(*it);
	}
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

		// Create the directory 
		if (fs::create_directory(dirPath))
		{
			// Directory has been created, set permissions
			rConsole() << "Directory " << dirPath << " created succesfully." << std::endl;

#ifdef DR_USE_STD_FILESYSTEM
			// Set permissions to rwxrwxr_x
			fs::permissions(dirPath, fs::perms::add_perms |
				fs::perms::owner_exec | fs::perms::owner_write | fs::perms::owner_read |
				fs::perms::group_exec | fs::perms::group_write | fs::perms::group_read |
				fs::perms::others_exec | fs::perms::others_read);
#else
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

} // namespace os
