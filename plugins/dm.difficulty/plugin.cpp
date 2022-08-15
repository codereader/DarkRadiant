#include "imodule.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include "ui/imenumanager.h"
#include "iregistry.h"
#include "iselection.h"
#include "iradiant.h"
#include "iundo.h"

#include "ClassNameStore.h"
#include "DifficultyDialog.h"

/**
 * Module to register the menu commands for the Difficulty Editor class.
 */
class DifficultyEditorModule :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	const std::string& getName() const
	{
		static std::string _name("DifficultyEditor");
		return _name;
	}

	const StringSet& getDependencies() const
	{
        static StringSet _dependencies
        {
            MODULE_MENUMANAGER,
            MODULE_COMMANDSYSTEM
        };

		return _dependencies;
	}

	void initialiseModule(const IApplicationContext& ctx)
	{
		// Add the callback event
		GlobalCommandSystem().addCommand("DifficultyEditor",  ui::DifficultyDialog::ShowDialog);

		// Add the menu item
        GlobalMenuManager().add("main/map", 	// menu location path
				"DifficultyEditor", // name
				ui::menu::ItemType::Item,	// type
				_("Difficulty..."),	// caption
				"stimresponse.png",	// icon
				"DifficultyEditor"); // event name
	}

	void shutdownModule()
	{
		ui::ClassNameStore::destroy();
	}
};
typedef std::shared_ptr<DifficultyEditorModule> DifficultyEditorModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(DifficultyEditorModulePtr(new DifficultyEditorModule));
}
