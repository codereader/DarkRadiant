#pragma once

#include <string>
#include <algorithm>
#include <cctype>

namespace string
{

/**
 * Converts the given input string to lowercase, using the 
 * C-function tolower(). The string is modified in-place.
 */
inline void to_lower(std::string& input)
{
	std::transform(input.begin(), input.end(), input.begin(), [](char c) { return(static_cast<char>(::tolower(c))); });
}

/**
* Converts the given input string to lowercase, using the
* C-function tolower(), and returns a copy of the result.
*/
inline std::string to_lower_copy(const std::string& input)
{
	std::string output;
	output.resize(input.size());

	std::transform(input.begin(), input.end(), output.begin(), [](char c) { return(static_cast<char>(::tolower(c))); });

	return output;
}

/**
* Converts the given input string to uppercase, using the
* C-function toupper(). The string is modified in-place.
*/
inline void to_upper(std::string& input)
{
	std::transform(input.begin(), input.end(), input.begin(), [](char c) { return(static_cast<char>(::toupper(c))); });
}

/**
* Converts the given input string to uppercase, using the
* C-function tolower(), and returns a copy of the result.
*/
inline std::string to_upper_copy(const std::string& input)
{
	std::string output;
	output.resize(input.size());

	std::transform(input.begin(), input.end(), output.begin(), [](char c) { return(static_cast<char>(::toupper(c))); });

	return output;
}

}
