#pragma once

/// \file
/// C-style null-terminated-character-array string library.

#include <cstring>

namespace string
{

/// \brief Returns <0 if \p string is lexicographically less than \p other after converting both to lower-case.
/// Returns >0 if \p string is lexicographically greater than \p other after converting both to lower-case.
/// Returns 0 if \p string is lexicographically equal to \p other after converting both to lower-case.
/// O(n)
inline int icmp(const char* string, const char* other)
{
#ifdef WIN32
  return _stricmp(string, other);
#else
  return strcasecmp(string, other);
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