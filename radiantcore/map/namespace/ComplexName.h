#pragma once

#include <string>
#include <set>

/// Set of unique postfixes, e.g. "1", "6" or "04"
typedef std::set<std::string> PostfixSet;

/// Name consisting of initial text and optional unique-making number-postfix 
/// e.g. "Carl" + "6", or "Mary" + "03"
class ComplexName
{
    // Initial name text
    std::string _name;

    // The number postfix in string form. "-" is considered an invalid/empty postfix
    std::string _postFix;

public:
	static const std::string EMPTY_POSTFIX; // "-"

    /// Construct a ComplexName from the given full name string
    ComplexName(const std::string& fullname);

    /// Get the full name in string format
    std::string getFullname() const;

    /// Get the initial text without any numeric postfix
    const std::string& getNameWithoutPostfix() const
    {
        return _name;
    }

    /// Get the numeric postfix ("-" indicates no postfix is used)
	std::string getPostfix() const
    {
        return _postFix;
    }

    /**
     * \brief
     * Change (if necessary) the postfix to make it unique, and return the new
     * postfix value.
     *
     * \param postfixes
     * Set of existing postfixes which must not be used.
     */
    std::string makePostfixUnique(const PostfixSet& postfixes);
};
