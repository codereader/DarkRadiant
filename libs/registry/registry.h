#pragma once

#include "iregistry.h"

/// Convenience methods and types for interacting with the XML registry
namespace registry
{

/**
 * \brief
 * Set a value in the registry of any type that can be converted to a string
 * with boost::lexical_cast.
 */
template<typename T> void setValue(const std::string& key, const T& value)
{
    try
    {
        GlobalRegistry().set(key, boost::lexical_cast<std::string>(value));
    }
    catch (const boost::bad_lexical_cast&)
    {
        GlobalRegistry().set(key, "");
    }
}

/**
 * \brief
 * Get the value of the given registry and convert it to type T. If the key
 * cannot be found or is not convertible to the required type, a
 * default-constructed T will be returned.
 *
 * T must be default-constructible, copy-constructible and convertible from
 * an std::string using boost::lexical_cast.
 */
template<typename T> T getValue(const std::string& key)
{
    try
    {
        if (GlobalRegistry().keyExists(key))
        {
            return boost::lexical_cast<T>(GlobalRegistry().get(key));
        }
    }
    catch (const boost::bad_lexical_cast&) { }

    return T();
}

}
