#pragma once

#include <boost/lexical_cast.hpp>

namespace string
{

/// Convert a string to a different type, with a fallback value
template<typename T, typename Src> T convert(const Src& str, T defaultVal = T())
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

#ifdef SPECIALISE_STR_TO_FLOAT
/**
 * \brief
 * Convert a string to a float.
 *
 * If the SPECIALISE_STR_TO_FLOAT macro is defined, this will use the atof() C
 * function instead of convert<float>() which may provide a speed benefit.
 *
 * \internal
 * This is a separate function rather than an actual specialisation of
 * convert<>() for two reasons: (1) I could not get the specialisation to work
 * (i.e. be chosen by the compiler), and (2) since when using atof() there is no
 * reliable way to use the fallback value, the default value argument would be
 * ignored and hence the user-visible function behaviour would change in the
 * specialised version, which is a bad thing.
 */
template<typename Src> double to_float(const Src& str)
{
    return std::atof(str.c_str());
}
#else
template<typename Src> float to_float(const Src& src)
{
    return convert<float>(src, 0.0f);
}
#endif

/// Convenient shortcut for convert<std::string>(T blah)
template<typename Src> std::string to_string(const Src& value)
{
    return convert<std::string>(value);
}

}
