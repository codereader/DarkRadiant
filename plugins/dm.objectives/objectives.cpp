#include "ObjectivesEditor.h"

#include "imodule.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "iradiant.h"
#include "iscenegraph.h"
#include "ieclass.h"
#include "ientity.h"
#include "itextstream.h"

#include "stream/textstream.h"
#include "generic/callback.h"

#include <iostream>

/**
 * \file objectives.cpp
 * Main plugin file for the Objectives Editor.
 * 
 * \namespace objectives
 * Classes and types comprising the Objectives Editor.
 */

/**
 * API module to register the menu commands for the ObjectivesEditor class.
 */
class ObjectivesEditorModule : 
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ObjectivesEditor");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_UIMANAGER);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "ObjectivesEditorModule::initialiseModule called.\n";
		
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
};
typedef boost::shared_ptr<ObjectivesEditorModule> ObjectivesEditorModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(ObjectivesEditorModulePtr(new ObjectivesEditorModule));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getErrorStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
