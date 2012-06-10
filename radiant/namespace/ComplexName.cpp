#include "ComplexName.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "string/convert.h"

ComplexName::ComplexName(const std::string& fullname)
{
    // Retrieve the name by cutting off the trailing number
    _name = boost::algorithm::trim_right_copy_if(
        fullname, boost::algorithm::is_any_of("1234567890")
    );

    // Get the trimmed part and take it as postfix
    std::string postFixStr = fullname.substr(_name.size());
    _postFix = string::convert<int>(postFixStr, -1);
}

std::string ComplexName::getFullname() const
{
    return _name + (_postFix == -1 ? "" : string::to_string(_postFix));
}

namespace
{
    // Find the first integer not in the given set
    int findFirstUnusedNumber(const PostfixSet& set)
    {
        static const int LOWEST_VAL = 1;

        for (int i = LOWEST_VAL; i < INT_MAX; ++i)
        {
            if (set.find(i) == set.end())
            {
                // Found an unused value
                return i;
            }
        }

        // Pathological case, could not find a value
        return INT_MAX;
    }
}

int ComplexName::makePostfixUnique(const PostfixSet& postfixes)
{
    // If our postfix is already in the set, change it to a unique value
    if (postfixes.find(_postFix) != postfixes.end())
    {
        _postFix = findFirstUnusedNumber(postfixes);
    }

    return _postFix;
}
