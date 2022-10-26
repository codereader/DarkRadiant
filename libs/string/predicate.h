#pragma once

#include <string>
#include <cstring>
#include <cctype>

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

} // detail

/**
* Compares the two strings for equality using the given comparator function.
* Returns true if the two strings are considered equal.
*/
template<typename Comparator>
inline bool equals(const std::string& str1, const std::string& str2, Comparator compare)
{
	std::string::const_iterator str1It = str1.begin();
	std::string::const_iterator str1End = str1.end();

	std::string::const_iterator str2It = str2.begin();
	std::string::const_iterator str2End = str2.end();

	for (; str1It != str1End && str2It != str2End; ++str1It, ++str2It)
	{
		if (!compare(*str1It, *str2It))
		{
			return false;
		}
	}

	return str1It == str1End && str2It == str2End;
}

/**
* Compares the two strings for equality using the given comparator function.
* Returns true if the two strings are considered equal.
*/
template<typename Comparator>
inline bool equals(const std::string& str1, const char* str2, Comparator compare)
{
	if (str2 == nullptr) return false;

	std::string::const_iterator str1It = str1.begin();
	std::string::const_iterator str1End = str1.end();

	const char* str2It = str2;

	for (; str1It != str1End && *str2It != '\0'; ++str1It, ++str2It)
	{
		if (!compare(*str1It, *str2It))
		{
			return false;
		}
	}

	return str1It == str1End && *str2It == '\0';
}

/**
* Returns true if the given input string is equal with the given test string,
* compared case-sensitively.
*/
template<typename TestStringType>
inline bool equals(const std::string& input, const TestStringType& test)
{
	return equals(input, test, detail::isEqual);
}

/**
* Returns true if the given input string is equal with the given test string,
* compared case-insensitively.
*/
template<typename TestStringType>
inline bool iequals(const std::string& input, const TestStringType& test)
{
	return equals(input, test, detail::isEqualNoCase);
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

/**
* Returns true if the given input string ends with the given test string,
* compared using the given Comparator functor.
*/
template<typename Comparator>
inline bool ends_with(const std::string& input, const std::string& test, Comparator compare)
{
	std::string::const_reverse_iterator inputEnd = input.rend();
	std::string::const_reverse_iterator testEnd = test.rend();
	std::string::const_reverse_iterator testIt = test.rbegin();

	for (std::string::const_reverse_iterator inpIt = input.rbegin();
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
* Returns true if the given input string ends with the given test string sequence,
* compared using the given Comparator functor.
*/
template<typename Comparator>
inline bool ends_with(const std::string& input, const char* test, Comparator compare)
{
	if (test == nullptr) return false;

	std::string::const_reverse_iterator inputEnd = input.rend();
	const char* testIt = test + std::strlen(test) - 1;

	for (std::string::const_reverse_iterator inpIt = input.rbegin();
		inpIt != inputEnd && testIt >= test;
		++inpIt, --testIt)
	{
		if (!compare(*inpIt, *testIt))
		{
			return false;
		}

		if (testIt == test)
		{
			return true;
		}
	}

	return false;
}

/**
* Returns true if the given input string ends with the given test string,
* compared case-sensitively.
*/
template<typename TestStringType>
inline bool ends_with(const std::string& input, const TestStringType& test)
{
	return ends_with(input, test, detail::isEqual);
}

/**
* Returns true if the given input string ends with the given test string,
* compared case-insensitively.
*/
template<typename TestStringType>
inline bool iends_with(const std::string& input, const TestStringType& test)
{
	return ends_with(input, test, detail::isEqualNoCase);
}

/**
 * Returns true if the given inpit string is consisting of alphanumeric characters only.
 * Returns false if the input string is an empty string.
 */
inline bool isAlphaNumeric(const std::string& input)
{
    for (const auto c : input)
    {
        if (!::isalpha(c) && !::isdigit(c))
        {
            return false;
        }
    }

    return !input.empty();
}

}
