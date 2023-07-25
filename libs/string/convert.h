#pragma once

#include "math/Vector3.h"
#include "math/Vector4.h"
#include <sstream>
#include <cstdlib>

namespace string
{

/**
 * @brief Convert a string to a different type, returning a default value if the conversion
 * did not succeed.
 *
 * @tparam T
 * Type of destination value. May be any type for which stringstream::operator>>() is
 * defined.
 */
template<typename T> T convert(const std::string& str, T defaultVal = {}) noexcept
{
    std::stringstream stream(str);
    T result;

    stream >> result;
    if (stream.fail())
        return defaultVal;
    else
        return result;
}

// Template specialisation to convert std::string => bool
// Returns the default value if the string is empty, everything else except "0" returns true
template<> inline bool convert<bool>(const std::string& str, bool defaultVal) noexcept
{
    return str.empty() ? defaultVal : (str != "0");
}

/**
 * @brief Identity "conversion" from std::string to std::string.
 *
 * Not much point in calling this explicitly, but might improve performance if found by
 * type-based lookup in a templated function that calls convert<T>(), since it does not
 * construct a stringstream.
 */
template<> inline std::string convert<std::string>(const std::string& str,
                                                    std::string defaultVal) noexcept
{
    return str;
}

#ifdef SPECIALISE_STR_TO_FLOAT
/**
 * \brief
 * Convert a string to a float.
 *
 * If the SPECIALISE_STR_TO_FLOAT macro is defined, this will use the atof() C
 * function instead of convert<float>() which may provide a speed benefit.
 *
 * \internal
 * This is a separate function rather than an actual specialisation of
 * convert<>() for two reasons: (1) I could not get the specialisation to work
 * (i.e. be chosen by the compiler), and (2) since when using atof() there is no
 * reliable way to use the fallback value, the default value argument would be
 * ignored and hence the user-visible function behaviour would change in the
 * specialised version, which is a bad thing.
 */
template<typename Src> double to_float(const Src& str)
{
    return std::atof(str.c_str());
}
#else
template<typename Src> float to_float(const Src& src)
{
    return convert<float>(src, 0.0f);
}
#endif

// Attempts to convert the given source string to a float value,
// returning true on success. The value reference will then be holding
// the resulting float value (or 0 in case of failure).
// Note: this is using the exception-less std::strtof, making it preferable
// over the string::convert<float> method (in certain hot code paths).
inline bool tryConvertToFloat(const std::string& src, float& value)
{
    char* lastChar;
    auto* firstChar = src.c_str();
    value = std::strtof(firstChar, &lastChar);

    return lastChar != firstChar;
}

// Attempts to convert the given source string to an int value,
// returning true on success. The value reference will then be holding
// the resulting int value (or 0 in case of failure).
// Note: this is using the exception-less std::strtol, making it preferable
// over the string::convert<int> method (in certain hot code paths).
inline bool tryConvertToInt(const std::string& src, int& value)
{
    char* lastChar;
    auto* firstChar = src.c_str();
    value = static_cast<int>(std::strtol(firstChar, &lastChar, 10));

    return lastChar != firstChar;
}

// Convert the given type to a std::string
template<typename Src>
inline std::string to_string(const Src& value)
{
    return std::to_string(value);
}

// Specialisation for Vector3
template<>
inline std::string to_string<Vector3>(const Vector3& value)
{
	std::stringstream str;
	str << value;
	return str.str();
}

// Specialisation for std::string
template<>
inline std::string to_string<std::string>(const std::string& value)
{
	return value;
}

// Special overload for converting const char* to string (this might actually
// happen in other template<T> functions calling to_string<T> with T = const char*
inline std::string to_string(const char* value)
{
	return value;
}

}
