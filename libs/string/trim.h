#pragma once

#include <string>

namespace string
{

/**
* Removes all characters from the beginning of the string matching the given predicate,
* modifying the given string in-place.
*/
template<typename Predicate>
inline void trim_left_if(std::string& subject, Predicate predicate)
{
	// Erase everything from the beginning up to the first character 
	// that is not matching the predicate (e.g. is not a space)
	subject.erase(subject.begin(), 
		std::find_if(subject.begin(), subject.end(), [&](int ch) { return !predicate(ch); }));
}

/**
* Removes all characters from the end of the string matching the given predicate,
* modifying the given string in-place.
*/
template<typename Predicate>
inline void trim_right_if(std::string& subject, Predicate predicate)
{
	// Erase everything from the end up to the nearest-to-last character 
	// that is not matching the predicate (e.g. is not a space)
	subject.erase(
		std::find_if(subject.rbegin(), subject.rend(), [&](int ch) { return !predicate(ch); }).base(),
		subject.end());
}

/**
* Removes all characters from the beginning and the end of the string 
* matching the given predicate, returning a new string containing of the result.
*/
template<typename Predicate>
inline std::string trim_copy_if(std::string subject, Predicate predicate)
{
	trim_left_if(subject, predicate);
	trim_right_if(subject, predicate);

	return subject;
}

/**
* Removes all space characters from the beginning and the end of the string, in-place.
*/
inline void trim(std::string& subject)
{
	trim_left_if(subject, ::isspace);
	trim_right_if(subject, ::isspace);
}

/**
* Removes all space characters from the beginning and the end of the string
* and returns a new string containing of the result.
*/
inline std::string trim_copy(std::string subject)
{
	trim_left_if(subject, ::isspace);
	trim_right_if(subject, ::isspace);

	return subject;
}

/**
* Removes all of the given characters from the beginning and the end of the subject, in-place.
*/
inline void trim(std::string& subject, const std::string& charsToBeRemoved)
{
	trim_left_if(subject, [&](int ch) { return charsToBeRemoved.find(ch) != std::string::npos; });
	trim_right_if(subject, [&](int ch) { return charsToBeRemoved.find(ch) != std::string::npos; });
}

/**
* Removes all of the given characters from the beginning and the end of the subject, 
* returning a new string containing the result.
*/
inline std::string trim_copy(std::string subject, const std::string& charsToBeRemoved)
{
	trim_left_if(subject, [&](int ch) { return charsToBeRemoved.find(ch) != std::string::npos; });
	trim_right_if(subject, [&](int ch) { return charsToBeRemoved.find(ch) != std::string::npos; });

	return subject;
}

/**
* Removes all of the given characters from the beginning of the subject, in-place
*/
inline void trim_left(std::string& subject, const std::string& charsToBeRemoved)
{
	trim_left_if(subject, [&](int ch) { return charsToBeRemoved.find(ch) != std::string::npos; });
}

/**
* Removes all of the given characters from the beginning of the subject,
* returning a new string containing the result.
*/
inline std::string trim_left_copy(std::string subject, const std::string& charsToBeRemoved)
{
	trim_left_if(subject, [&](int ch) { return charsToBeRemoved.find(ch) != std::string::npos; });

	return subject;
}

/**
* Removes all of the given characters from the end of the subject, in-place
*/
inline void trim_right(std::string& subject, const std::string& charsToBeRemoved)
{
	trim_right_if(subject, [&](int ch) { return charsToBeRemoved.find(ch) != std::string::npos; });
}

/**
* Removes all of the given characters from the end of the subject,
* returning a new string containing the result.
*/
inline std::string trim_right_copy(std::string subject, const std::string& charsToBeRemoved)
{
	trim_right_if(subject, [&](int ch) { return charsToBeRemoved.find(ch) != std::string::npos; });

	return subject;
}

}
