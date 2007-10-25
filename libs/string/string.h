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

#if !defined(INCLUDED_STRING_STRING_H)
#define INCLUDED_STRING_STRING_H

/// \file
/// C-style null-terminated-character-array string library.

#include <cstring>
#include <cctype>
#include <algorithm>

#include "memory/allocator.h"
#include "generic/arrayrange.h"

#include <boost/lexical_cast.hpp>

inline std::string intToStr(const int& i) {
	std::string returnValue;
	
	try {
		returnValue = boost::lexical_cast<std::string>(i);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue = "";
	}
	
	return returnValue;
}

inline int strToInt(const std::string& str) {
	int returnValue;
	
	try {
		returnValue = boost::lexical_cast<int>(str);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue = 0;
	}
	
	return returnValue;
}

inline std::string floatToStr(const float& f) {
	std::string returnValue;
	
	try {
		returnValue = boost::lexical_cast<std::string>(f);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue = "";
	}
	
	return returnValue;
}

inline float strToFloat(const std::string& str) {
	float returnValue;
	
	try {
		returnValue = boost::lexical_cast<float>(str);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue = 0;
	}
	
	return returnValue;
}

inline std::string doubleToStr(const double& f) {
	std::string returnValue;
	
	try {
		returnValue = boost::lexical_cast<std::string>(f);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue = "";
	}
	
	return returnValue;
}

inline double strToDouble(const std::string& str) {
	double returnValue;
	
	try {
		returnValue = boost::lexical_cast<double>(str);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue = 0;
	}
	
	return returnValue;
}

inline std::string sizetToStr(const std::size_t& s) {
	std::string returnValue;
	
	try {
		returnValue = boost::lexical_cast<std::string>(s);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue = "";
	}
	
	return returnValue;
}

inline std::size_t strToSizet(const std::string& str) {
	std::size_t returnValue;
	
	try {
		returnValue = boost::lexical_cast<std::size_t>(str);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue = 0;
	}
	
	return returnValue;
}

/// \brief Returns true if \p string length is zero.
/// O(1)
inline bool string_empty(const char* string)
{
  return *string == '\0';
}

/// \brief Returns true if \p string length is not zero.
/// O(1)
inline bool string_not_empty(const char* string)
{
  return !string_empty(string);
}

/// \brief Returns <0 if \p string is lexicographically less than \p other.
/// Returns >0 if \p string is lexicographically greater than \p other.
/// Returns 0 if \p string is lexicographically equal to \p other.
/// O(n)
inline int string_compare(const char* string, const char* other)
{
  return std::strcmp(string, other);
}

/// \brief Returns true if \p string is lexicographically equal to \p other.
/// O(n)
inline bool string_equal(const char* string, const char* other)
{
  return string_compare(string, other) == 0;
}

/// \brief Returns true if [\p string, \p string + \p n) is lexicographically equal to [\p other, \p other + \p n).
/// O(n)
inline bool string_equal_n(const char* string, const char* other, std::size_t n)
{
  return std::strncmp(string, other, n) == 0;
}

/// \brief Returns true if \p string is lexicographically less than \p other.
/// O(n)
inline bool string_less(const char* string, const char* other)
{
  return string_compare(string, other) < 0;
}

/// \brief Returns true if \p string is lexicographically greater than \p other.
/// O(n)
inline bool string_greater(const char* string, const char* other)
{
  return string_compare(string, other) > 0;
}

/// \brief Returns <0 if \p string is lexicographically less than \p other after converting both to lower-case.
/// Returns >0 if \p string is lexicographically greater than \p other after converting both to lower-case.
/// Returns 0 if \p string is lexicographically equal to \p other after converting both to lower-case.
/// O(n)
inline int string_compare_nocase(const char* string, const char* other)
{
#ifdef WIN32
  return _stricmp(string, other);
#else
  return strcasecmp(string, other);
#endif
}

/// \brief Returns <0 if [\p string, \p string + \p n) is lexicographically less than [\p other, \p other + \p n).
/// Returns >0 if [\p string, \p string + \p n) is lexicographically greater than [\p other, \p other + \p n).
/// Returns 0 if [\p string, \p string + \p n) is lexicographically equal to [\p other, \p other + \p n).
/// Treats all ascii characters as lower-case during comparisons.
/// O(n)
inline int string_compare_nocase_n(const char* string, const char* other, std::size_t n)
{
#ifdef WIN32
  return _strnicmp(string, other, n);
#else
  return strncasecmp(string, other, n);
#endif
}

/// \brief Returns true if \p string is lexicographically equal to \p other.
/// Treats all ascii characters as lower-case during comparisons.
/// O(n)
inline bool string_equal_nocase(const char* string, const char* other)
{
  return string_compare_nocase(string, other) == 0;
}

/// \brief Returns true if [\p string, \p string + \p n) is lexicographically equal to [\p other, \p other + \p n).
/// Treats all ascii characters as lower-case during comparisons.
/// O(n)
inline bool string_equal_nocase_n(const char* string, const char* other, std::size_t n)
{
  return string_compare_nocase_n(string, other, n) == 0;
}

/// \brief Returns true if \p string is lexicographically less than \p other.
/// Treats all ascii characters as lower-case during comparisons.
/// O(n)
inline bool string_less_nocase(const char* string, const char* other)
{
  return string_compare_nocase(string, other) < 0;
}

/// \brief Returns the number of non-null characters in \p string.
/// O(n)
inline std::size_t string_length(const char* string)
{
  return std::strlen(string);
}

#endif
