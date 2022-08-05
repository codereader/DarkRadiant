#pragma once

#include "math/Vector3.h"
#include "math/Vector4.h"
#include <sstream>
#include <cstdlib>

namespace string
{

// Converts a string to a different type, with a fallback value
// A couple of template specialisations are defined below.
template<typename T, typename Src> 
T convert(const Src& str, T defaultVal = T());

// Template specialisation to convert std::string => float
template<>
inline float convert<float, std::string>(const std::string& str, float defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : std::stof(str);
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => double
template<>
inline double convert<double, std::string>(const std::string& str, double defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : std::stod(str);
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => short
template<>
inline short convert<short, std::string>(const std::string& str, short defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : static_cast<short>(std::stoi(str));
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => unsigned short
template<>
inline unsigned short convert<unsigned short, std::string>(const std::string& str, unsigned short defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : static_cast<unsigned short>(std::stoul(str));
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => int
template<>
inline int convert<int, std::string>(const std::string& str, int defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : std::stoi(str);
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => unsigned int
template<>
inline unsigned int convert<unsigned int, std::string>(const std::string& str, unsigned int defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : static_cast<unsigned int>(std::stoul(str));
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => long
template<>
inline long convert<long, std::string>(const std::string& str, long defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : std::stol(str);
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => long long
template<>
inline long long convert<long long, std::string>(const std::string& str, long long defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : std::stoll(str);
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => unsigned long
template<>
inline unsigned long convert<unsigned long, std::string>(const std::string& str, unsigned long defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : std::stoul(str);
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => unsigned long long
template<>
inline unsigned long long convert<unsigned long long, std::string>(const std::string& str, unsigned long long defaultVal)
{
	try
	{
		return str.empty() ? defaultVal : std::stoull(str);
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => bool
// Returns the default value if the string is empty, everything else except "0" returns true
template<>
inline bool convert<bool, std::string>(const std::string& str, bool defaultVal)
{
	return str.empty() ? defaultVal : (str != "0");
}

// Template specialisation to extract a Vector3 from the given string
template<>
inline Vector3 convert<Vector3, std::string>(const std::string& str, Vector3 defaultVal)
{
    // Quickly return if nothing to parse
    if (str.empty()) return defaultVal;

	try
	{
		Vector3 vec;

		std::istringstream stream(str);
		stream >> std::skipws >> vec.x() >> vec.y() >> vec.z();

		if (stream.fail()) throw std::invalid_argument("Failed to parse Vector3");

		return vec;
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to extract a Vector4 from the given string
template<>
inline Vector4 convert<Vector4, std::string>(const std::string& str, Vector4 defaultVal)
{
    // Quickly return if nothing to parse
    if (str.empty()) return defaultVal;

	try
	{
		Vector4 vec;

		std::istringstream stream(str);
		stream >> std::skipws >> vec.x() >> vec.y() >> vec.z() >> vec.w();

		if (stream.bad()) throw std::invalid_argument("Failed to parse Vector4");

		return vec;
	}
	catch (const std::logic_error&) // logic_error is base of invalid_argument out_of_range exceptions
	{
		return defaultVal;
	}
}

// Template specialisation to convert std::string => std::string, returning the input value
template<>
inline std::string convert<std::string, std::string>(const std::string& str, std::string defaultVal)
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
