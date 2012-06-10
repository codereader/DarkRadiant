#pragma once

#include <string>
#include <set>

/// Set of unique integer postfixes
typedef std::set<int> PostfixSet;

/// Name consisting of initial text and optional unique-making numeric postfix
class ComplexName
{
    // Initial name text
    std::string _name;

    // Numeric postfix. -1 is considered as invalid/empty postfix
    int _postFix;

public:

    /// Construct a ComplexName from the given full name string
    ComplexName(const std::string& fullname);

    /// Get the full name in string format
    std::string getFullname() const;

    /// Get the initial text without any numeric postfix
    const std::string& getNameWithoutPostfix() const
    {
        return _name;
    }

    /// Get the numeric postfix (-1 indicates no postfix is used)
    int getPostfix() const
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
    int makePostfixUnique(const PostfixSet& postfixes);
};
