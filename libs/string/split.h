#pragma once

#include <string>
#include <iterator>

namespace string
{

/**
 * Splits the given string into pieces based on the given delimiters, storing
 * the tokens at the end the given token container (which might be any container 
 * supporting insert iterators (std::vector, std::list, std::set, etc.)
 */
template<class ContainerType>
inline void split(ContainerType& tokens, const std::string& subject, const std::string& delimiters, bool trimEmpty = true)
{
	std::string::size_type lastPos = 0;
	std::string::size_type length = subject.length();

	// Construct an output iterator which inserts elements at the end of the container
	std::insert_iterator<ContainerType> output = std::inserter(tokens, tokens.end());

	while (lastPos < length + 1)
	{
		// Find the next occurrence of any of the delimiters
		std::string::size_type pos = subject.find_first_of(delimiters, lastPos);

		// Nothing found, set the pos to trigger an exit
		if (pos == std::string::npos)
		{
			pos = length;
		}

		// If the found position is different from the last occurrence, we have a non-empty token
		// If the position is the same, this is an empty token, in which case we need to check the flag
		if (pos != lastPos || !trimEmpty)
		{
			// Copy the range [lastPos..pos] from the subject and insert it into our token container
			output = ContainerType::value_type(
				subject.data() + lastPos, static_cast<ContainerType::size_type>(pos - lastPos));
		}

		// Advance the cursor by one to skip the found delimiter
		lastPos = pos + 1;
	}
}

}
