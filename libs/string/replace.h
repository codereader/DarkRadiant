#pragma once

#include <string>

namespace string
{

/** 
 * Replaces all occurrences of the given search string in the subject
 * with the given replacement, in-place.
 */
inline void replace_all(std::string& subject, const std::string& search, const std::string& replacement)
{
	std::size_t pos = 0;

	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replacement);
		pos += replacement.length();
	}
}

/**
 * Replaces all occurrences of of the given search string with 
 * the given replacement and returns a new string instance 
 * containing the result. The incoming subject is passed by value such
 * that the original string is not altered.
 */
inline std::string replace_all_copy(std::string subject, const std::string& search, const std::string& replacement)
{
	std::size_t pos = 0;

	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replacement);
		pos += replacement.length();
	}

	return subject;
}

}
