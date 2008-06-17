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

#ifndef _OS_DIR_H_
#define _OS_DIR_H_

/// \file
/// \brief OS directory-listing object.

#include <glib/gdir.h>

#include <string>
#include <stdexcept>

typedef GDir Directory;

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

/**
 * Exception thrown by Directory_forEach to indicate that the given directory
 * does not exist.
 */
class DirectoryNotFoundException
: public std::runtime_error
{
public:
    DirectoryNotFoundException(const std::string& what)
    : std::runtime_error(what)
    { }
};

// Invoke functor for all items in a directory
template<typename Functor>
void Directory_forEach(const std::string& path, Functor& functor) {
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
}


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

#ifdef WIN32

#include <direct.h>

namespace os {

// returns true if succeeded in creating directory
inline bool makeDirectory(const std::string& name) {
	return _mkdir(name.c_str()) != -1; 
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

#endif /* _OS_DIR_H_ */
