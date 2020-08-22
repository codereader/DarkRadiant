#pragma once

#include "registry.h"

#include <sigc++/bind.h>
#include <sigc++/connection.h>

namespace registry
{

namespace detail
{
    inline void invokeFromBoolean(const std::string& key,
                                  sigc::slot<void> trueCallback,
                                  sigc::slot<void> falseCallback)
    {
        if (getValue<bool>(key))
            trueCallback();
        else
            falseCallback();
    }
}

/**
 * \brief
 * Adaptor function to connect two slots to a boolean registry key, with one
 * slot invoked when the value changes to true and the other invoked when the
 * value changes to false.
 */
inline sigc::connection observeBooleanKey(const std::string& key,
                              sigc::slot<void> trueCallback,
                              sigc::slot<void> falseCallback)
{
    return GlobalRegistry().signalForKey(key).connect(
        sigc::bind(sigc::ptr_fun(&detail::invokeFromBoolean),
                   key,
                   trueCallback,
                   falseCallback)
    );
}

}
