#ifndef SHADERNAMECOMPAREFUNCTOR_H_
#define SHADERNAMECOMPAREFUNCTOR_H_

#include <map>
#include <string>
#include <boost/algorithm/string/predicate.hpp>

/**
 * allow case-insensitive binary searchs with shader definitions.
 */
struct ShaderNameCompareFunctor : public std::binary_function<std::string, std::string, bool>
{
	bool operator()(const std::string &s1, const std::string &s2) const {
		return boost::algorithm::ilexicographical_compare(s1, s2);
	}
};

#endif /*SHADERNAMECOMPAREFUNCTOR_H_*/
