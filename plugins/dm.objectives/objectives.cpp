#include "ObjectivesEditor.h"

#include "iplugin.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "iradiant.h"
#include "iscenegraph.h"
#include "typesystem.h"
#include "ieclass.h"
#include "ientity.h"

#include "generic/callback.h"

#include <iostream>

/**
 * API module to register the menu commands for the ObjectivesEditor class.
 */
class ObjectivesEditorAPI
: public IPlugin
{
public:
	STRING_CONSTANT(Name, "ObjectivesEditor");
	typedef IPlugin Type;

	/**
	 * API constructor, registers the commands in the Event manager and UI
	 * manager.
	 */
	ObjectivesEditorAPI() {
		
		// Add the callback event
		GlobalEventManager().addCommand(
			"ObjectivesEditor",
			FreeCaller<objectives::ObjectivesEditor::displayDialog>());
	
	
		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/map", 
				"ObjectivesEditor", 
				ui::menuItem,
				"Objectives...",
				"objectives16.png",
				"ObjectivesEditor");
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
struct ObjectivesEditorDependencies
: public GlobalEntityClassManagerModuleRef,
  public GlobalEntityModuleRef,
  public GlobalSceneGraphModuleRef,
  public GlobalEventManagerModuleRef,
  public GlobalRadiantModuleRef,
  public GlobalUIManagerModuleRef,
  public TypeSystemRef
{ 
	// Constructor. We must specify a game type for the eclassmanager and
	// entity creator, since these do not match "*" lookups
	ObjectivesEditorDependencies()
	: GlobalEntityClassManagerModuleRef("doom3"),
	  GlobalEntityModuleRef("doom3")
	{ }
};

/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<ObjectivesEditorAPI,
						ObjectivesEditorDependencies> ObjectivesEditorModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
	// Static module instance
	static ObjectivesEditorModule _instance;
	
	// Initialise and register the module	
	initialiseModule(server);
	_instance.selfRegister();
}
