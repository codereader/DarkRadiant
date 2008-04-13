#ifndef REGISTRYTOGGLE_H_
#define REGISTRYTOGGLE_H_

#include "ieventmanager.h"
#include "iregistry.h"
#include "Toggle.h"

/* greebo: A RegistryToggle is an Toggle Event that changes the value of the
 * attached registry key to "1" / "0" when toggled. The key is stored internally of course. 
 * 
 * The class functions as a RegistryKeyObserver, so it gets notified on key changes.
 * 
 * The only method that is different to an ordinary Toggle is the virtual toggle() method,
 * whose only purpose is to toggle the according RegistryKey.
 */
class RegistryToggle :
	public Toggle,
	public RegistryKeyObserver
{
	// The attached registrykey
	const std::string _registryKey;
	
public:

	RegistryToggle(const std::string& registryKey) :
		Toggle(MemberCaller<RegistryToggle, &RegistryToggle::doNothing>(*this)),
		_registryKey(registryKey)
	{
		// Initialise the current state
		_toggled = (GlobalRegistry().get(_registryKey) == "1");
		
		// Register self as KeyObserver to get notified on key changes
		GlobalRegistry().addKeyObserver(this, _registryKey);
	}

	virtual ~RegistryToggle() {}
	
	// Dummy callback for the Toggle base class, we don't need any callbacks...
	void doNothing() {}

	virtual bool setToggled(const bool toggled) {
		// Set the registry key, this triggers the keyChanged() method
		GlobalRegistry().set(_registryKey, toggled ? "1" : "0");
		
		return true;
	}

	// The RegistryKeyObserver implementation, gets called on key changes
	void keyChanged(const std::string& key, const std::string& val) {
		// Update the internal toggle state according to the key value 
		_toggled = (val == "1");
		
		updateWidgets();
	}
	
	virtual void toggle() {
		if (_callbackActive) {
			return;
		}
		
		// Check if the toggle event is enabled
		if (_enabled) {
			// Invert the registry key to <toggled> state 
			GlobalRegistry().set(_registryKey, (GlobalRegistry().get(_registryKey) == "1") ? "0" : "1");
			
			// The updates of the widgets are done by the triggered keyChanged() method
		}
	}

}; // class RegistryTogle

#endif /*REGISTRYTOGGLE_H_*/
