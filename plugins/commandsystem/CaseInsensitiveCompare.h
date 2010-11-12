#ifndef _CASE_INSENSITIVE_COMPARE_FUNCTOR_H_
#define _CASE_INSENSITIVE_COMPARE_FUNCTOR_H_

#include <map>
#include <string>
#include <boost/algorithm/string/predicate.hpp>

namespace cmd {

/**
 * Compare-functor to allow case-insensitive lookups of commands.
 */
struct CaseInsensitiveCompare :
	public std::binary_function<std::string, std::string, bool>
{
	bool operator()(const std::string &s1, const std::string &s2) const {
		return boost::algorithm::ilexicographical_compare(s1, s2);
	}
};

} // namespace cmd

#endif /* _CASE_INSENSITIVE_COMPARE_FUNCTOR_H_ */
