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

#if !defined(INCLUDED_FS_PATH_H)
#define INCLUDED_FS_PATH_H

#include <boost/algorithm/string/predicate.hpp>

/// \brief A unix-style path string which can be modified at runtime.
///
/// - Maintains a path ending in a path-separator.
/// - Provides a limited STL-style interface to push and pop file or directory names at the end of the path.

// greebo: TODO: Perform a profile run with a lot of VFS traversals to check the performance of this class
class UnixPath
{
	// The actual string representation of the path
	std::string _string;

	void check_separator() {
		// If the string is non-empty, but hasn't a trailing slash, add it
		if (!_string.empty() && !boost::algorithm::ends_with(_string, "/")) {
			_string.push_back('/');
		}
	}

public:
	/// \brief Constructs with the directory \p root.
	UnixPath(const std::string& root) :
		_string(root)
	{
		check_separator();
	}

	bool empty() const {
		return _string.empty();
	}

	const char* c_str() const {
		return _string.c_str();
	}

	/// \brief Appends the directory \p name.
	void push(const char* name) {
		_string += name;
		check_separator();
	}
	/// \brief Appends the directory [\p first, \p last).
	void push(const char* first, const char* last) {
		for (const char* i = first; i != last; i++) {
			_string.push_back(*i);
		}
		check_separator();
	}
	
	/// \brief Appends the filename \p name.
	void push_filename(const char* name) {
		_string += name;
	}
	
	/// \brief Removes the last directory or filename appended.
	// greebo: TODO: This looks slow
	void pop() {
		// Remove the trailing slash
		if (boost::algorithm::ends_with(_string, "/")) {
			_string.erase(_string.length()-1, 1);
		}
		
		// Remove all characters, back to front, till we hit a slash
		while (!_string.empty() && !boost::algorithm::ends_with(_string, "/")) {
			_string.erase(_string.length()-1, 1);
		}
	}
};

#endif
