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

/// \brief A unix-style path string which can be modified at runtime.
///
/// - Maintains a path ending in a path-separator.
/// - Provides a limited STL-style interface to push and pop file or directory names at the end of the path.
class UnixPath
{
	// The actual string representation of the path
	std::string _string;
	std::size_t _size; // cached size

	void update() {
		// If the string is non-empty, but hasn't a trailing slash, add it
		_size = _string.size();
		if (_size > 0 && _string[_size-1] != '/') {
			_string.push_back('/');
			_size++;
		}
	}

public:
	/// \brief Constructs with the directory \p root.
	UnixPath(const std::string& root) :
		_string(root)
	{
		update();
	}

	bool empty() const {
		return (_size == 0);
	}

	const char* c_str() const {
		return _string.c_str();
	}
	
	operator const std::string& () const {
		return _string;
	}
	
	/// \brief Appends the directory \p name.
	void push(const std::string& name) {
		_string += name;
		update();
	}

	/// \brief Appends the directory [\p first, \p last).
	void push(const char* first, const char* last) {
		for (const char* i = first; i != last; i++) {
			_string.push_back(*i);
		}
		update();
	}
	
	/// \brief Appends the filename \p name.
	void push_filename(const std::string& name) {
		_string += name;
		_size = _string.size();
	}
	
	/// \brief Removes the last directory or filename appended.
	void pop() {
		// Remove the trailing slash
		if (_size > 0 && _string[_size-1] == '/') {
			_string.erase(_size-1, 1);
		}
		// _size is considered invalid after this point
		
		std::size_t slashPos = _string.rfind("/");
		if (slashPos != std::string::npos) {
			// Erase everything after the slashpos
			_string.erase(slashPos+1);
		}
		else {
			// No slash at all, clear everything
			_string.clear();
		}
		
		_size = _string.size();
	}
};

#endif
