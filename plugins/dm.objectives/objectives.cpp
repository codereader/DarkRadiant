#include "ObjectivesEditor.h"

#include "iplugin.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "qerplugin.h"
#include "iscenegraph.h"
#include "typesystem.h"

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

	ObjectivesEditorAPI() {
		
		// Add the callback event
		GlobalEventManager().addCommand("ObjectivesEditor",
							FreeCaller<ui::ObjectivesEditor::displayDialog>());
	
	
		// Add the menu item
		IMenuManager* mm = GlobalUIManager().getMenuManager();
		mm->add("main/map", 
				"ObjectivesEditor", 
				ui::menuItem,
				"Objectives...",
				"objectives16.png",
				"ObjectivesEditor");
		std::cout << "ObjectivesEditor constructed." << std::endl;
	}
	
	IPlugin* getTable() {
		return this;
	}	
};

/**
 * Dependencies class.
 */
class ObjectivesEditorDependencies
: public GlobalEventManagerModuleRef,
  public GlobalUIManagerModuleRef,
  public GlobalRadiantModuleRef,
  public GlobalSceneGraphModuleRef,
  public TypeSystemRef
{ };

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
