#include "imodule.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include "icommandsystem.h"
#include "iuimanager.h"
#include "iregistry.h"
#include "iselection.h"
#include "iradiant.h"
#include "iundo.h"
#include "i18n.h"

#include "debugging/debugging.h"
#include "StimResponseEditor.h"

/**
 * Module to register the menu commands for the Stim/Response Editor class.
 */
class StimResponseModule :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("StimResponseEditor");
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
		rMessage() << "StimResponseModule::initialiseModule called.\n";

		// Add the callback event
		GlobalCommandSystem().addCommand("StimResponseEditor", ui::StimResponseEditor::showDialog);
		GlobalEventManager().addCommand("StimResponseEditor", "StimResponseEditor");

		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/entity", 	// menu location path
				"StimResponse", // name
				ui::menuItem,	// type
				_("Stim/Response..."),	// caption
				"stimresponse.png",	// icon
				"StimResponseEditor"); // event name
	}
};
typedef boost::shared_ptr<StimResponseModule> StimResponseModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(StimResponseModulePtr(new StimResponseModule));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
