#include "imodule.h"

#include "i18n.h"
#include "itextstream.h"
#include "icommandsystem.h"
#include "ui/imenumanager.h"
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

	virtual const StringSet& getDependencies() const
    {
        static StringSet _dependencies
        {
            MODULE_MENUMANAGER,
            MODULE_COMMANDSYSTEM
        };

		return _dependencies;
	}

	virtual void initialiseModule(const IApplicationContext& ctx) {

		// Add the callback event
		GlobalCommandSystem().addCommand("ConversationEditor", ui::ConversationDialog::ShowDialog);

		// Add the menu item
        GlobalMenuManager().add("main/map", 	// menu location path
				"ConversationEditor", // name
				ui::menu::ItemType::Item,	// type
				_("Conversations..."),	// caption
				"stimresponse.png",	// icon
				"ConversationEditor"); // event name
	}
};
typedef std::shared_ptr<ConversationEditorModule> ConversationEditorModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(ConversationEditorModulePtr(new ConversationEditorModule));
}
