#include "ComplexName.h"

#include <climits>
#include "string/trim.h"
#include "string/convert.h"

const std::string ComplexName::EMPTY_POSTFIX("-");

ComplexName::ComplexName(const std::string& fullname)
{
    // Retrieve the name by cutting off the trailing number
    _name = string::trim_right_copy(fullname, "1234567890");

    // Get the trimmed part and take it as postfix
    std::string postFixStr = fullname.substr(_name.size());

	// Assign the empty Postfix placeholder if we didn't find any numbers
	_postFix = !postFixStr.empty() ? postFixStr : EMPTY_POSTFIX;
}

std::string ComplexName::getFullname() const
{
    return _name + (_postFix != EMPTY_POSTFIX ? _postFix : "");
}

namespace
{
    // Find the first integer not in the given set
    std::string findFirstUnusedNumber(const PostfixSet& set)
    {
        static const int LOWEST_VAL = 1;

        for (int i = LOWEST_VAL; i < INT_MAX; ++i)
        {
			std::string testPostfix = string::to_string(i);

            if (set.find(testPostfix) == set.end())
            {
                // Found an unused value
                return testPostfix;
            }
        }

        // Pathological case, could not find a value
        return string::to_string(INT_MAX);
    }
}

std::string ComplexName::makePostfixUnique(const PostfixSet& postfixes)
{
    // If our postfix is already in the set, change it to a unique value
    if (postfixes.find(_postFix) != postfixes.end())
    {
        _postFix = findFirstUnusedNumber(postfixes);
    }

    return _postFix;
}
