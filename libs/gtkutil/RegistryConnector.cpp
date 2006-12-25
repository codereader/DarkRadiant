#include "RegistryConnector.h"

#include "iregistry.h"
#include <iostream>
#include "gtk/gtktogglebutton.h"
#include "gtk/gtkcombobox.h"
#include "gtk/gtkentry.h"

namespace gtkutil {

// Connects the given GtkObject to the passed registryKey
void RegistryConnector::connectGtkObject(GtkObject* object, const std::string& registryKey) {
	
	// Add the GtkObject to the internal list
	_objectKeyMap[object] = registryKey;
	
	// Initialise the value of the GtkObject by importing it from the registry
	importKey(object, registryKey);
}

void RegistryConnector::importKey(GtkObject* obj, const std::string& registryKey) {
	
	if (GTK_IS_TOGGLE_BUTTON(obj)) {
		// Set the "active" state of the toggle button according to the registry value
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), GlobalRegistry().get(registryKey)=="1");
	}
	else if (GTK_IS_ADJUSTMENT(obj)) {
		// Set the value of the adjustment according to the registry value
		gtk_adjustment_set_value(GTK_ADJUSTMENT(obj), GlobalRegistry().getFloat(registryKey));
	}
	else if (GTK_IS_COMBO_BOX(obj)) {
		// Set the "active" state of the combo box according to the registry value
		gtk_combo_box_set_active(GTK_COMBO_BOX(obj), GlobalRegistry().getInt(registryKey));
	}
	else if (GTK_IS_ENTRY(obj)) {
		// Set the content of the input field to the registryKey
		gtk_entry_set_text(GTK_ENTRY(obj), GlobalRegistry().get(registryKey).c_str());
	}
	else {
		std::cout << "RegistryConnector::importKey failed to identify GTKObject\n";
	}
}

// Retrieve the value from the GtkObject and save it into the registry
void RegistryConnector::exportKey(GtkObject* obj, const std::string& registryKey) {
	
	if (GTK_IS_TOGGLE_BUTTON(obj)) {
		// Set the registry key to "1" or "0", according on the toggle button state
		GlobalRegistry().set(registryKey, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(obj)) ? "1" : "0");
	}
	else if (GTK_IS_ADJUSTMENT(obj)) {
		// Store the new value into the registry
		GlobalRegistry().setFloat(registryKey, gtk_adjustment_get_value(GTK_ADJUSTMENT(obj)));
	}
	else if (GTK_IS_COMBO_BOX(obj)) {
		// Store the value into the registry
		GlobalRegistry().setInt(registryKey, gtk_combo_box_get_active(GTK_COMBO_BOX(obj)));
	}
	else if (GTK_IS_ENTRY(obj)) {
		// Get the content of the input field and write it to the registry
		GlobalRegistry().set(registryKey, gtk_entry_get_text(GTK_ENTRY(obj)));
	}
	else {
		std::cout << "RegistryConnector::exportKey failed to identify GTKObject\n";
	}
}

void RegistryConnector::importValues() {
	for (ObjectKeyMap::iterator i = _objectKeyMap.begin(); i != _objectKeyMap.end(); i++) {
		// Call the importer method with the GtkObject and the registryKey
		importKey(i->first, i->second);
	}
}

void RegistryConnector::exportValues() {
	for (ObjectKeyMap::iterator i = _objectKeyMap.begin(); i != _objectKeyMap.end(); i++) {
		// Call the export method that takes care of the specific GtkObject type
		exportKey(i->first, i->second);
	}
}

} // namespace xml
