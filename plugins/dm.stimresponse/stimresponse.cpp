#include "iplugin.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "qerplugin.h"

#include "generic/callback.h"
#include "gtkutil/dialog.h"

// Test command
void testCmd() {
	gtkutil::errorDialog("Stim/Response is not implemented yet.",
						 GlobalRadiant().getMainWindow());
}

/**
 * API module to register the menu commands for the ObjectivesEditor class.
 */
class StimResponseAPI
: public IPlugin
{
public:
	STRING_CONSTANT(Name, "StimResponse");
	typedef IPlugin Type;

	/**
	 * API constructor, registers the commands in the Event manager and UI
	 * manager.
	 */
	StimResponseAPI() {
		
		// Add the callback event
		GlobalEventManager().addCommand("StimResponse", FreeCaller<testCmd>());
	
		// Add the menu item
		IMenuManager* mm = GlobalUIManager().getMenuManager();
		mm->add("main/entity", 
				"StimResponse", 
				ui::menuItem,
				"Stim/Response...",
				"stimresponse.png",
				"StimResponse");
	}
	
	/**
	 * SingletonModule requires a getTable() method, although for plugins it is
	 * unused.
	 */
	IPlugin* getTable() {
		assert(false);
	}	
};

/**
 * Dependencies class.
 */
class StimResponseDependencies
: public GlobalEventManagerModuleRef,
  public GlobalUIManagerModuleRef,
  public GlobalRadiantModuleRef
{ };

/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<StimResponseAPI,
						StimResponseDependencies> StimResponseModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
	// Static module instance
	static StimResponseModule _instance;
	
	// Initialise and register the module	
	initialiseModule(server);
	_instance.selfRegister();
}
