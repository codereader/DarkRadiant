#pragma once

#include "ieventmanager.h"
#include "Toggle.h"

#include "registry/adaptors.h"
#include <boost/bind.hpp>

/* greebo: A RegistryToggle is an Toggle Event that changes the value of the
 * attached registry key to "1" / "0" when toggled. The key is stored internally of course.
 *
 * The only method that is different to an ordinary Toggle is the virtual toggle() method,
 * whose only purpose is to toggle the according RegistryKey.
 */
class RegistryToggle : public Toggle,
                       public sigc::trackable
{
private:
	// The attached registrykey
	const std::string _registryKey;

	void setState(bool state)
	{
		_toggled = state;
		updateWidgets();
	}

public:

	RegistryToggle(const std::string& registryKey) :
		Toggle(boost::bind(&RegistryToggle::doNothing, this, _1)),
		_registryKey(registryKey)
	{
		// Initialise the current state
		_toggled = registry::getValue<bool>(_registryKey);

		// Register to get notified on key changes
        registry::observeBooleanKey(
            _registryKey,
            sigc::bind(sigc::mem_fun(this, &RegistryToggle::setState), true),
            sigc::bind(sigc::mem_fun(this, &RegistryToggle::setState), false)
        );

	}

	// Dummy callback for the Toggle base class, we don't need any callbacks...
	void doNothing(bool) {}

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
