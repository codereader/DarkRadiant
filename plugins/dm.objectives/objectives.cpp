#include "ObjectivesEditor.h"

#include "i18n.h"
#include "imodule.h"
#include "ui/imenumanager.h"
#include "iradiant.h"
#include "iscenegraph.h"
#include "ieclass.h"
#include "ientity.h"
#include "itextstream.h"

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
			_dependencies.insert(MODULE_MENUMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const IApplicationContext& ctx) {

		// Add the callback event
		GlobalCommandSystem().addCommand(
			"ObjectivesEditor",
			objectives::ObjectivesEditor::DisplayDialog
		);

		// Add the menu item
		ui::menu::IMenuManager& mm = GlobalMenuManager();
		mm.add("main/map",
				"ObjectivesEditor",
				ui::menu::ItemType::Item,
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
typedef std::shared_ptr<ObjectivesEditorModule> ObjectivesEditorModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(ObjectivesEditorModulePtr(new ObjectivesEditorModule));
}
