#pragma once

#include "ieventmanager.h"
#include "Toggle.h"

#include "registry/adaptors.h"
#include <functional>
#include <sigc++/connection.h>

namespace ui
{

/* greebo: A RegistryToggle is a Toggle Event that changes the value of the
 * attached registry key to "1" / "0" when toggled. The key is stored internally.
 *
 * The only method different to an ordinary Toggle is the virtual toggle() method,
 * whose only purpose is to toggle the according RegistryKey.
 */
class RegistryToggle : 
    public Toggle,
    public sigc::trackable
{
private:
    // The attached registrykey
    const std::string _registryKey;

    sigc::connection _registryConn;

    void setState(bool state)
    {
        _toggled = state;
        updateWidgets();
    }

    // Dummy callback for the Toggle base class, we don't need any callbacks...
    static void doNothing(bool) {}

public:

    RegistryToggle(const std::string& registryKey) :
        Toggle(doNothing),
        _registryKey(registryKey)
    {
        // Initialise the current state
        _toggled = registry::getValue<bool>(_registryKey);

        // Register to get notified on key changes
        _registryConn = registry::observeBooleanKey(
            _registryKey,
            sigc::bind(sigc::mem_fun(this, &RegistryToggle::setState), true),
            sigc::bind(sigc::mem_fun(this, &RegistryToggle::setState), false)
        );
    }

    ~RegistryToggle()
    {
        _registryConn.disconnect();
    }

    virtual bool setToggled(const bool toggled)
    {
        // Set the registry key, this triggers the setState() method
        registry::setValue(_registryKey, toggled);

        return true;
    }

    virtual void toggle()
    {
        if (_callbackActive) {
            return;
        }

        // Check if the toggle event is enabled
        if (_enabled)
        {
            // Invert the registry key to <toggled> state
            registry::setValue(
                _registryKey, !registry::getValue<bool>(_registryKey)
                );

            // The updates of the widgets are done by the triggered
            // setState() method
        }
    }

}; // class RegistryTogle

}
