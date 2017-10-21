#pragma once

#include <string>

namespace string
{

/**
* Joins all the parts of the given container into a new string, separating
* the parts by the given separator string.
*/
template<typename ContainerType>
inline std::string join(const ContainerType& parts, const std::string& separator)
{
	std::string result;

	if (parts.empty()) return result;

	typename ContainerType::const_iterator part = parts.begin();
	result.append(*part++);

	while (part != parts.end())
	{
		result.append(separator);
		result.append(*part++);
	}

	return result;
}

}
