#pragma once

/// \file
/// C-style null-terminated-character-array string library.

#include <cstring>

namespace string
{

/**
 * @brief Case-insensitive string comparison.
 *
 * Behaves like the standard strcmp() but ignores case.
 *
 * @return A negative integer if lhs is lexicographically less than rhs (ignoring case), 0 if both
 * are equal, or a positive integer if lhs is greater than rhs.
 */
inline int icmp(const char* lhs, const char* rhs)
{
#ifdef WIN32
  return _stricmp(lhs, rhs);
#else
  return strcasecmp(lhs, rhs);
#endif
}

/// Case-insensitive comparison functor for use with data structures
struct ILess
{
    bool operator() (const std::string& lhs, const std::string& rhs) const
    {
        return icmp(lhs.c_str(), rhs.c_str()) < 0;
    }
};

}

/// \brief Returns true if [\p string, \p string + \p n) is lexicographically equal to [\p other, \p other + \p n).
/// O(n)
inline bool string_equal_n(const char* string, const char* other, std::size_t n)
{
  return std::strncmp(string, other, n) == 0;
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

/// \brief Returns true if [\p string, \p string + \p n) is lexicographically equal to [\p other, \p other + \p n).
/// Treats all ascii characters as lower-case during comparisons.
/// O(n)
inline bool string_equal_nocase_n(const char* string, const char* other, std::size_t n)
{
    return string_compare_nocase_n(string, other, n) == 0;
}

/// \brief Returns true if \p string is lexicographically equal to \p other.
/// Treats all ascii characters as lower-case during comparisons.
/// O(n)
inline bool string_equal_nocase(const char* string, const char* other)
{
  return string::icmp(string, other) == 0;
}

/// \brief Returns true if \p string is lexicographically less than \p other.
/// Treats all ascii characters as lower-case during comparisons.
/// O(n)
inline bool string_less_nocase(const char* string, const char* other)
{
  return string::icmp(string, other) < 0;
}