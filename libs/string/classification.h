#pragma once

#include <string>

namespace string
{

namespace detail
{

inline bool isEqual(const std::string::value_type& a, const std::string::value_type& b)
{
	return a == b;
}

inline bool isEqualNoCase(const std::string::value_type& a, const std::string::value_type& b)
{
	return ::tolower(a) == ::tolower(b);
}

}

/**
* Returns true if the given input string starts with the given test string,
* compared using the given Comparator functor.
*/
template<typename Comparator>
inline bool starts_with(const std::string& input, const std::string& test, Comparator compare)
{
	std::string::const_iterator inputEnd = input.end();
	std::string::const_iterator testEnd = test.end();
	std::string::const_iterator testIt = test.begin();

	for (std::string::const_iterator inpIt = input.begin();
		inpIt != inputEnd && testIt != testEnd; 
		++inpIt, ++testIt)
	{
		if (!compare(*inpIt, *testIt))
		{
			return false;
		}
	}

	return testIt == testEnd;
}

/**
* Returns true if the given input string starts with the given test string sequence,
* compared using the given Comparator functor.
*/
template<typename Comparator>
inline bool starts_with(const std::string& input, const char* test, Comparator compare)
{
	if (test == nullptr) return false;

	std::string::const_iterator inputEnd = input.end();
	const char* testIt = test;

	for (std::string::const_iterator inpIt = input.begin();
		inpIt != inputEnd && *testIt != '\0';
		++inpIt, ++testIt)
	{
		if (!compare(*inpIt, *testIt))
		{
			return false;
		}
	}

	return *testIt == '\0';
}

/**
* Returns true if the given input string starts with the given test string,
* compared case-sensitively.
*/
template<typename TestStringType>
inline bool starts_with(const std::string& input, const TestStringType& test)
{
	return starts_with(input, test, detail::isEqual);
}

/**
* Returns true if the given input string starts with the given test string,
* compared case-insensitively.
*/
template<typename TestStringType>
inline bool istarts_with(const std::string& input, const TestStringType& test)
{
	return starts_with(input, test, detail::isEqualNoCase);
}

}
