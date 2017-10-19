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
#pragma once

/// \file
/// \brief OS file-system path comparison, decomposition and manipulation.
///
/// - Paths are c-style null-terminated-character-arrays.
/// - Path separators must be forward slashes (unix style).
/// - Directory paths must end in a separator.
/// - Paths must not contain the ascii characters \\ : * ? " < > or |.
/// - Paths may be encoded in UTF-8 or any extended-ascii character set.

#include "string/string.h"

#include "string/predicate.h"
#include "string/replace.h"

#if defined(WIN32)
#define OS_CASE_INSENSITIVE
#endif

/// \brief Returns true if the first \p n bytes of \p path and \p other form paths that refer to the same file or directory.
/// If the paths are UTF-8 encoded, [\p path, \p path + \p n) must be a complete path.
/// O(n)
inline bool path_equal_n(const char* path, const char* other, std::size_t n)
{
#if defined(OS_CASE_INSENSITIVE)
  return string_equal_nocase_n(path, other, n);
#else
  return string_equal_n(path, other, n);
#endif
}


/// \brief Returns true if \p path is a fully qualified file-system path.
/// O(1)
inline bool path_is_absolute(const char* path)
{
#if defined(WIN32)
  return path[0] == '/'
    || (path[0] != '\0' && path[1] == ':'); // local drive
#elif defined(POSIX)
  return path[0] == '/';
#endif
}

/// \brief Returns a pointer to the first character of the component of \p path following the first directory component.
/// O(n)
inline const char* path_remove_directory(const char* path)
{
  const char* first_separator = strchr(path, '/');
  if(first_separator != 0)
  {
    return ++first_separator;
  }
  return "";
}

/** General utility functions for OS-related tasks
 */
namespace os
{
    /** Convert the slashes in a Doom 3 path to forward-slashes. Doom 3 accepts either
     * forward or backslashes in its definitions
     */

    inline std::string standardPath(const std::string& inPath)
	{
        return string::replace_all_copy(inPath, "\\", "/");
    }

    /** greebo: OS Folder names have forward slashes and a trailing slash
     * 			at the end by convention.
     */
    inline std::string standardPathWithSlash(const std::string& input) {
		std::string output = standardPath(input);

		// Append a slash at the end, if there isn't already one
		if (!string::ends_with(output, "/")) {
			output += "/";
		}
		return output;
	}

    /**
     * Return the path of fullPath relative to basePath, as long as fullPath
     * is contained within basePath. If not, fullPath is returned unchanged.
     */
    inline std::string getRelativePath(const std::string& fullPath,
                                       const std::string& basePath)
    {
#ifdef OS_CASE_INSENSITIVE
		if (string::istarts_with(fullPath, basePath))
#else
		if (string::starts_with(fullPath, basePath))
#endif
		{
			return fullPath.substr(basePath.length());
        }
        else {
            return fullPath;
        }
    }

	/**
	 * stifu: Does the same as getRelativePath, but also strips the filename.
	 */
	inline std::string getRelativePathMinusFilename(const std::string& fullPath,
												 const std::string& basePath)
	{
#ifdef OS_CASE_INSENSITIVE
		if (string::istarts_with(fullPath, basePath))
#else
		if (string::starts_with(fullPath, basePath))
#endif
		{
			return fullPath.substr(basePath.length(), fullPath.rfind('/') - basePath.length());
        }
        else {
            return fullPath;
        }
	}


    /**
     * greebo: Get the filename contained in the given path (the part after the last slash).
     * If there is no filename, an empty string is returned.
     *
     * Note: The input string is expected to be standardised (forward slashes).
     */
    inline std::string getFilename(const std::string& path) {
        std::size_t slashPos = path.rfind('/');
        if (slashPos == std::string::npos) {
            return "";
        }
        else {
            return path.substr(slashPos + 1);
        }
    }

    /**
     * Get the extension of the given filename. If there is no extension, an
     * empty string is returned.
     */
    inline std::string getExtension(const std::string& path) {
        std::size_t dotPos = path.rfind('.');
        if (dotPos == std::string::npos) {
            return "";
        }
        else {
            return path.substr(dotPos + 1);
        }
    }

    /**
     * Get the containing folder of the specified object. This is calculated
     * as the directory before the rightmost slash (which will be the object
     * itself, if the pathname ends in a slash).
     *
     * If the path does not contain a slash, the empty string will be returned.
     *
     * E.g.
     * blah/bleh/file   -> "bleh"
     * blah/bloog/      -> "bloog"
     */
    inline std::string getContainingDir(const std::string& path) {
        std::size_t lastSlash = path.rfind('/');
        if (lastSlash == std::string::npos) {
            return "";
        }
        std::string trimmed = path.substr(0, lastSlash);
        lastSlash = trimmed.rfind('/');
        return trimmed.substr(lastSlash + 1);
    }

	/**
	 * Returns the directory part of the given path, cutting off the filename
	 * right after the last slash. Use standard paths (forward slashes) only!
	 *
	 * E.g.
	 * blah/bleh/file.ext   -> "blah/bleh/"
	 * blah/bloog			-> "blah/"
	 */
	inline std::string getDirectory(const std::string& path)
	{
		std::size_t lastSlash = path.rfind('/');

		if (lastSlash == std::string::npos)
		{
			return path;
		}

		return path.substr(0, lastSlash + 1);
	}

	/**
	 * Returns true if the given string qualifies as path to a directory,
	 * which is equal to the path string ending with the slash '/' character.
	 */
	inline bool isDirectory(const std::string& path)
	{
		return !path.empty() && path.back() == '/';
	}
}
