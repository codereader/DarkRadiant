#pragma once

#include <boost/lexical_cast.hpp>

namespace string
{

/// Convert a string to a different type, with a fallback value
template<typename T, typename Src>
T convert(const Src& str, T defaultVal = T())
{
    try
    {
        return boost::lexical_cast<T>(str);
    }
    catch (const boost::bad_lexical_cast&)
    {
        return defaultVal;
    }
}

}
