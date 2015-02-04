#pragma once

/// \file
/// \brief OS directory-listing object.
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#include <string>
#include <stdexcept>

#if 0
inline bool directory_good(Directory* directory)
{
  return directory != 0;
}

inline Directory* directory_open(const std::string& name)
{
  return g_dir_open(name.c_str(), 0, 0);
}

inline void directory_close(Directory* directory)
{
  g_dir_close(directory);
}

inline const char* directory_read_and_increment(Directory* directory)
{
  return g_dir_read_name(directory);
}
#endif

// greebo: Moved this from GtkRadiant's cmdlib.h to here
// some easy portability crap

#define access_owner_read 0400
#define access_owner_write 0200
#define access_owner_execute 0100
#define access_owner_rw_ 0600
#define access_owner_r_x 0500
#define access_owner__wx 0300
#define access_owner_rwx 0700

#define access_group_read 0040
#define access_group_write 0020
#define access_group_execute 0010
#define access_group_rw_ 0060
#define access_group_r_x 0050
#define access_group__wx 0030
#define access_group_rwx 0070

#define access_others_read 0004
#define access_others_write 0002
#define access_others_execute 0001
#define access_others_rw_ 0006
#define access_others_r_x 0005
#define access_others__wx 0003
#define access_others_rwx 0007

#define access_rwxrwxr_x (access_owner_rwx | access_group_rwx | access_others_r_x)
#define access_rwxrwxrwx (access_owner_rwx | access_group_rwx | access_others_rwx)

namespace os
{

/**
 * Exception thrown by Directory_forEach to indicate that the given directory
 * does not exist.
 */
class DirectoryNotFoundException
: public std::runtime_error
{
public:
    DirectoryNotFoundException(const std::string& what) : 
		std::runtime_error(what)
    {}
};

// Invoke functor for all items in a directory
inline void foreachItemInDirectory(const std::string& path, const std::function<void (const fs::path&)>& functor)
{
	fs::path start(path);

	if (!fs::exists(start))
	{
		throw DirectoryNotFoundException(
            "foreachItemInDirectory(): invalid directory '" + path + "'"
        );
	}

	for (fs::directory_iterator it(start); it != fs::directory_iterator(); ++it)
	{
		const fs::path& candidate = *it;

		functor(candidate);
	}
#if 0
	Directory* dir = directory_open(path);

	if (directory_good(dir)) {
		for (;;) {
			const char* name = directory_read_and_increment(dir);
			if (name == 0) {
				break;
			}

			functor(name);
		}

		directory_close(dir);
	}
    else {
        throw DirectoryNotFoundException(
            "Directory_forEach(): invalid directory '" + path + "'"
        );
    }
#endif
}

} // namespace

#ifdef WIN32

#include <direct.h>

namespace os
{

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
	int mkVal = _mkdir(name.c_str());
    if (mkVal == -1)
    {
        int err;
        _get_errno(&err);

        if (err == EEXIST)
        {
            return true;
        }
        else
        {
            rConsoleError() << "os::makeDirectory(" << name << ") failed with error "
                      << err << std::endl;
            return false;
        }
    }
    return true;
}

} // namespace os

#else // POSIX

#include <sys/stat.h>

namespace os {

// returns true if succeeded in creating directory
inline bool makeDirectory(const std::string& name) {
	return mkdir(name.c_str(), access_rwxrwxr_x) != -1;
}

} // namespace os

#endif
