#ifndef REGISTRYCONNECTOR_H_
#define REGISTRYCONNECTOR_H_

#include <map>
#include <string>
#include "iregistry.h"

/* greebo: This class establishes connections between values stored in GtkWidgets and
 * registry key/values. It provides methods for connecting the widget to certain keys,
 * and loading/saving the values from/to the registry. */

// Forward declarations to avoid including the whole GTK headers
typedef struct _GtkToggleButton GtkToggleButton;

namespace gtkutil {

	namespace {
		typedef std::map<GtkToggleButton*, std::string> ToggleButtonMap;
	}

class RegistryConnector {

	// The association of ToggleButtons and Registry keys
	ToggleButtonMap _toggleButtons;

public:
	// Connect a toggle button widget with the specified registryKey
	void connectToggleButton(GtkToggleButton* toggleButton, const std::string& registryKey);
	
	// Loads all the values from the registry into the connected GtkWidgets
	void importValues();
	
	// Reads the values from the GtkWidgets and writes them into the XMLRegistry
	void exportValues();

private:
	// Imports/Exports the values for the connected toggle buttons
	void importToggleButtonValues();
	void exportToggleButtonValues();

}; // class RegistryConnector

} // namespace xml

#endif /* REGISTRYCONNECTOR_H_ */
