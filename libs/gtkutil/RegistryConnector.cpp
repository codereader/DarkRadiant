#include "RegistryConnector.h"

#include "iregistry.h"
#include <iostream>
#include "gtk/gtktogglebutton.h"
#include <boost/lexical_cast.hpp>

namespace gtkutil {

void RegistryConnector::connectToggleButton(GtkToggleButton* toggleButton, const std::string& registryKey) {
	_toggleButtons[toggleButton] = registryKey;
	
	// Initialise the value of the button
	gtk_toggle_button_set_active(toggleButton, GlobalRegistry().get(registryKey)=="1");
}

void RegistryConnector::connectAdjustment(GtkAdjustment* adjustment, const std::string& registryKey) {
	_adjustments[adjustment] = registryKey;
	
	// Initialise the adjustment value
	gtk_adjustment_set_value(adjustment, GlobalRegistry().getFloat(registryKey));
}

void RegistryConnector::importAdjustmentValues() {
	for (AdjustmentMap::iterator i = _adjustments.begin(); i != _adjustments.end(); i++) {
		GtkAdjustment* adjustment = i->first;
		const std::string registryKey = i->second;
		
		// Set the value of the adjustment according to the registry value
		gtk_adjustment_set_value(adjustment, GlobalRegistry().getFloat(registryKey));
	}
}

void RegistryConnector::exportAdjustmentValues() {
	for (AdjustmentMap::iterator i = _adjustments.begin(); i != _adjustments.end(); i++) {
		GtkAdjustment* adjustment = i->first;
		const std::string registryKey = i->second;
		
		// Retrieve the current value of the adjustment
		double adjustmentValue = gtk_adjustment_get_value(adjustment);
		
		// Store the new value into the registry
		GlobalRegistry().setFloat(registryKey, adjustmentValue);
	}
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
	importAdjustmentValues();
}

void RegistryConnector::exportValues() {
	exportToggleButtonValues();
	exportAdjustmentValues();
}

} // namespace xml
