#pragma once

#include "iregistry.h"
#include "string/convert.h"
#include "string/predicate.h"

#include "util/Noncopyable.h"

/// Convenience methods and types for interacting with the XML registry
namespace registry
{

/**
 * \brief
 * Set a value in the registry of any type that can be converted to a string
 * with string::to_string.
 */
template<typename T> void setValue(const std::string& key, const T& value)
{
    GlobalRegistry().set(key, string::to_string(value));
}

/**
 * \brief Get the value of the given registry and convert it to type T. If the key cannot be found
 * or is not convertible to the required type, a default-constructed T will be returned.
 *
 * T must be default-constructible, copy-constructible and convertible from an std::string using
 * string::convert.
 */
template<typename T> T getValue(const std::string& key, T defaultVal = T())
{
    if (GlobalRegistry().keyExists(key))
        return string::convert<T>(GlobalRegistry().get(key));
    else
        return defaultVal;
}

// Returns the concatenated paths, separated by a slash
// The base path may or may not end with a slash
// The subpath must not start with a slash
inline std::string combinePath(std::string basePath, const std::string& subPath)
{
    return string::ends_with(basePath, "/") ? basePath + subPath : basePath + "/" + subPath;
}

/**
 * \brief
 * Scoped sentry object which sets a registry key to a temporary value for its
 * lifetime, and restores the original value on destruction.
 */
template<typename T>
class ScopedKeyChanger
: util::Noncopyable
{
    std::string _key;
    T _origVal;

public:

    ScopedKeyChanger(const std::string& key, T tempVal)
    : _key(key),
      _origVal(registry::getValue<T>(key))
    {
        registry::setValue(_key, tempVal);
    }

    ~ScopedKeyChanger()
    {
        registry::setValue(_key, _origVal);
    }
};

}
