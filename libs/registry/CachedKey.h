#pragma once

#include "registry.h"

namespace registry
{

/**
 * \brief
 * Simple value caching class for a registry key.
 *
 * This can be used when a registry key needs to be read frequently but only
 * written rarely. It stores a copy of the value converted to its given type,
 * and uses the registry signal to update its copy when the value changes.
 */
template<typename T>
class CachedKey: public sigc::trackable
{
    const std::string _key;
    T _cachedValue;

private:

    void updateCachedValue()
    {
        _cachedValue = registry::getValue<T>(_key);
    }

public:

    /// Construct a CachedKey to observe the given registry key
    CachedKey(const std::string& key)
    : _key(key)
    {
        updateCachedValue();

        GlobalRegistry().signalForKey(key).connect(
            sigc::mem_fun(this, &CachedKey<T>::updateCachedValue)
        );
    }

    /// Return the current value
    T get() const
    {
        return _cachedValue;
    }
};

}
