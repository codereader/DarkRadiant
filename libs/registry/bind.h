#pragma once

#include "registry.h"

#include <glibmm/propertyproxy.h>

namespace registry
{

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
 * Note that the observation happens in one direction only: the key is updated
 * to match the property. If the key could be changed somewhere else and the
 * property needs to be aware, use the Registry::signalForKey() signal.
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
