#ifndef REGISTRYCONNECTOR_H_
#define REGISTRYCONNECTOR_H_

#include <map>
#include <string>
#include "iregistry.h"

/* greebo: This class establishes connections between values stored in GtkObjects and
 * registry key/values. It provides methods for connecting the objects to certain keys,
 * and loading/saving the values from/to the registry. 
 * 
 * The references to the GtkWidgets/GtkObjects are stored as GtkObject* pointers, that
 * are cast onto the supported types (e.g. GtkToggleButton, GtkAdjustment, etc.).
 * 
 * The stored objects have to be supported by this class, otherwise an error is thrown to std::cout
 * 
 * Note to avoid any confusion:
 * 
 * "import" is to be understood as importing values FROM the XMLRegistry and storing them TO the widgets.
 * "export" is naturally the opposite: values are exported TO the XMLRegistry (with values FROM the widgets).
 * */

// Forward declarations to avoid including the whole GTK headers
typedef struct _GtkObject GtkObject;

namespace gtkutil {

	namespace {
		typedef std::map<GtkObject*, std::string> ObjectKeyMap;
	}

class RegistryConnector {

	// The association of widgets and registry keys
	ObjectKeyMap _objectKeyMap;

public:
	// Connect a GtkObject to the specified XMLRegistry key
	void connectGtkObject(GtkObject* object, const std::string& registryKey);
	
	// Loads all the values from the registry into the connected GtkObjects
	void importValues();
	
	// Reads the values from the GtkObjects and writes them into the XMLRegistry
	void exportValues();

private:
	// Load the value from the registry and store it into the GtkObject
	void importKey(GtkObject* obj, const std::string& registryKey);

	// Retrieve the value from the GtkObject and save it into the registry
	void exportKey(GtkObject* obj, const std::string& registryKey);

}; // class RegistryConnector

} // namespace gtkutil

#endif /* REGISTRYCONNECTOR_H_ */
