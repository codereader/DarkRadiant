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

namespace detail
{
    template<typename T>
    void savePropertyToRegistry(Glib::PropertyProxy<T>& prop,
                                const std::string& rkey)
    {
        setValue<T>(rkey, prop.get_value());
    }
}

/**
 * \brief
 * Connect a Glib property to a particular registry key value.
 *
 * This is used to persist a Glib property such as widget size to a key in the
 * registry. The registry key will be updated automatically when the property
 * changes, via the property's signal_changed() signal. If the registry key
 * exists at the time this function is called, the property will be set to the
 * current key value before the connection is made.
 *
 * \param prop
 * A Glib::PropertyProxy to be connected to the registry.
 *
 * \param key
 * The registry key that the property should be connected to.
 */
template<typename T>
void bindPropertyToKey(Glib::PropertyProxy<T> prop, const std::string& key)
{
    // Set initial value then connect to changed signal
    if (GlobalRegistry().keyExists(key))
    {
        prop.set_value(registry::getValue<T>(key));
    }
    prop.signal_changed().connect(
        sigc::bind(sigc::ptr_fun(detail::savePropertyToRegistry<T>), prop, key)
    );
}

}
