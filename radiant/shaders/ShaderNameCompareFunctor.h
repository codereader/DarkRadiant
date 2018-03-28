#pragma once

#include <map>
#include <string>
#include "string/string.h"

/**
 * allow case-insensitive binary searchs with shader definitions.
 */
struct ShaderNameCompareFunctor :
	public std::binary_function<std::string, std::string, bool>
{
	bool operator()(const std::string& s1, const std::string& s2) const
	{
		// return boost::algorithm::ilexicographical_compare(s1, s2); // slow!
		return string_compare_nocase(s1.c_str(), s2.c_str()) < 0;
	}
};
