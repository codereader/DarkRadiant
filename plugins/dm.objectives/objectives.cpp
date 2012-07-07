#include "ObjectivesEditor.h"

#include "i18n.h"
#include "imodule.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "iradiant.h"
#include "iscenegraph.h"
#include "ieclass.h"
#include "ientity.h"
#include "itextstream.h"
#include "debugging/debugging.h"

#include "ce/ComponentEditorFactory.h"
#include <iostream>

/**
 * \defgroup objectives Objectives Editor (Dark Mod only)
 *
 * \file objectives.cpp
 * Main plugin file for the Objectives Editor.
 *
 * \namespace objectives
 * \ingroup objectives
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
			_dependencies.insert(MODULE_COMMANDSYSTEM);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) {
		rMessage() << "ObjectivesEditorModule::initialiseModule called.\n";

		// Add the callback event
		GlobalCommandSystem().addCommand(
			"ObjectivesEditor",
			objectives::ObjectivesEditor::displayDialog
		);
		GlobalEventManager().addCommand("ObjectivesEditor", "ObjectivesEditor");

		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/map",
				"ObjectivesEditor",
				ui::menuItem,
				_("Objectives..."),
				"objectives16.png",
				"ObjectivesEditor");
	}

	virtual void shutdownModule() {
		rMessage() << "ObjectivesEditorModule shutting down.\n";

		// Remove all the registered Component Editors from memory
		objectives::ce::ComponentEditorFactory::clear();
	}
};
typedef boost::shared_ptr<ObjectivesEditorModule> ObjectivesEditorModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(ObjectivesEditorModulePtr(new ObjectivesEditorModule));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
