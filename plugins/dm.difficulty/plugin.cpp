#include "imodule.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "itextstream.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include "iuimanager.h"
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
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
		}

		return _dependencies;
	}

	void initialiseModule(const ApplicationContext& ctx)
	{
		rMessage() << getName() << "::initialiseModule called." << std::endl;

		// Add the callback event
		GlobalCommandSystem().addCommand("DifficultyEditor",  ui::DifficultyDialog::showDialog);
		GlobalEventManager().addCommand("DifficultyEditor", "DifficultyEditor");

		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/map", 	// menu location path
				"DifficultyEditor", // name
				ui::menuItem,	// type
				_("Difficulty..."),	// caption
				"stimresponse.png",	// icon
				"DifficultyEditor"); // event name
	}

	void shutdownModule()
	{
		ui::ClassNameStore::destroy();
	}
};
typedef boost::shared_ptr<DifficultyEditorModule> DifficultyEditorModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	registry.registerModule(DifficultyEditorModulePtr(new DifficultyEditorModule));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
