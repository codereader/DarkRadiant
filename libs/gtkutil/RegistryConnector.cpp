#include "RegistryConnector.h"

#include "iregistry.h"
#include <iostream>
#include "gtk/gtktogglebutton.h"

namespace gtkutil {

void RegistryConnector::connectToggleButton(GtkToggleButton* toggleButton, const std::string& registryKey) {
	_toggleButtons[toggleButton] = registryKey;
	
	// Initialise the value of the button
	gtk_toggle_button_set_active(toggleButton, GlobalRegistry().get(registryKey)=="1");
}

void RegistryConnector::importToggleButtonValues() {
	for (ToggleButtonMap::iterator i = _toggleButtons.begin(); i != _toggleButtons.end(); i++) {
		GtkToggleButton* toggleButton = i->first;
		const std::string registryKey = i->second;
		
		// Set the "active" state of the toggle button according to the registry value
		gtk_toggle_button_set_active(toggleButton, GlobalRegistry().get(registryKey)=="1");
	}
}

void RegistryConnector::exportToggleButtonValues() {
	for (ToggleButtonMap::iterator i = _toggleButtons.begin(); i != _toggleButtons.end(); i++) {
		GtkToggleButton* toggleButton = i->first;
		const std::string registryKey = i->second;
		
		// Retrieve the current state of the toggle button
		const std::string valueStr = gtk_toggle_button_get_active(toggleButton) ? "1" : "0";
		
		// Store the new value into the registry
		GlobalRegistry().set(registryKey, valueStr);
	}
}

void RegistryConnector::importValues() {
	importToggleButtonValues();
}

void RegistryConnector::exportValues() {
	exportToggleButtonValues();
}

} // namespace xml
