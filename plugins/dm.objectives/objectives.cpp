#include "iplugin.h"

#include <iostream>

class ObjectivesEditor
: public IPlugin
{
public:
	STRING_CONSTANT(Name, "ObjectivesEditor");
	typedef IPlugin Type;

	ObjectivesEditor() {
		std::cout << "ObjectivesEditor constructed." << std::endl;
	}
	
	IPlugin* getTable() {
		return this;
	}	
};


/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<ObjectivesEditor> ObjectivesEditorModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
	// Static module instance
	static ObjectivesEditorModule _instance;
	
	// Initialise and register the module	
	initialiseModule(server);
	_instance.selfRegister();
}
