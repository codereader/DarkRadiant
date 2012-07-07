#include "imodule.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "icommandsystem.h"
#include "iuimanager.h"
#include "generic/callback.h"
#include "debugging/debugging.h"

#include "ConversationDialog.h"

/**
 * Module to register the menu commands for the Conversation Editor class.
 */
class ConversationEditorModule :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ConversationEditor");
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
		rMessage() << getName() << "::initialiseModule called.\n";

		// Add the callback event
		GlobalCommandSystem().addCommand("ConversationEditor", ui::ConversationDialog::showDialog);
		GlobalEventManager().addCommand("ConversationEditor", "ConversationEditor");

		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/map", 	// menu location path
				"ConversationEditor", // name
				ui::menuItem,	// type
				_("Conversations..."),	// caption
				"stimresponse.png",	// icon
				"ConversationEditor"); // event name
	}
};
typedef boost::shared_ptr<ConversationEditorModule> ConversationEditorModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(ConversationEditorModulePtr(new ConversationEditorModule));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
