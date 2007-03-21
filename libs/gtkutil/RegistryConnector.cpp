#include "RegistryConnector.h"

#include "iregistry.h"
#include <iostream>
#include "gtk/gtktogglebutton.h"
#include "gtk/gtkcombobox.h"
#include "gtk/gtkentry.h"
#include "gtk/gtkspinbutton.h"
#include "gtk/gtkrange.h"
#include "gtk/gtkscale.h"
#include "gtk/gtkradiobutton.h"

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
		// Set the content of the entry field to the registryKey
		gtk_entry_set_text(GTK_ENTRY(obj), GlobalRegistry().get(registryKey).c_str());
	}
	else if (GTK_IS_SPIN_BUTTON(obj)) {
		// Set the content of the spinbuttonto the registryKey
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), GlobalRegistry().getFloat(registryKey));
	}
	else if (GTK_IS_SCALE(obj)) {
		// Set the content of the scale to the registryKey
		gtk_range_set_value(GTK_RANGE(obj), GlobalRegistry().getFloat(registryKey));
	}
	else if (GTK_IS_RADIO_BUTTON(obj)) {
		// Set the active index of the radio group to the registryKey
		GSList* group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(obj));
		int i = 0;
		int activeIndex = GlobalRegistry().getInt(registryKey);
		
		// Cycle through the radio button group and mark the active index as TOGGLED
		while (group != NULL) {
			if (activeIndex == i) {
				GtkToggleButton* radio = reinterpret_cast<GtkToggleButton*>(group->data);
				gtk_toggle_button_set_active(radio, TRUE);
			}
			group = group->next;
			i++;
		}
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
	else if (GTK_IS_SPIN_BUTTON(obj)) {
		// Set the content of the input field to the registryKey
		GlobalRegistry().setFloat(registryKey, gtk_spin_button_get_value(GTK_SPIN_BUTTON(obj)));
	}
	else if (GTK_IS_SCALE(obj)) {
		// Set the content of the input field to the registryKey
		GlobalRegistry().setFloat(registryKey, gtk_range_get_value(GTK_RANGE(obj)));
	}
	else if (GTK_IS_RADIO_BUTTON(obj)) {
		// Get the active index from the radio group
		GSList* group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(obj));
		int index = 0;
		// Cycle through the radio button group and mark the active index as TOGGLED
		while (group != NULL) {
			GtkToggleButton* radio = reinterpret_cast<GtkToggleButton*>(group->data);
			if (gtk_toggle_button_get_active(radio)) {
				GlobalRegistry().setInt(registryKey, index);
				break;
			}
			group = group->next;
			index++;
		}
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

} // namespace gtkutil
