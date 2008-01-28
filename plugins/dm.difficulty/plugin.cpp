#include "imodule.h"

#include "ieventmanager.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include "iuimanager.h"
#include "iregistry.h"
#include "iselection.h"
#include "iradiant.h"
#include "iundo.h"
#include "stream/textstream.h"
#include "generic/callback.h"

#include "DifficultyDialog.h"

/**
 * Module to register the menu commands for the Difficulty Editor class.
 */
class DifficultyEditorModule : 
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("DifficultyEditor");
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
		globalOutputStream() << getName().c_str() << "::initialiseModule called.\n";
		
		// Add the callback event
		GlobalEventManager().addCommand(
			"DifficultyEditor", 
			FreeCaller<ui::DifficultyDialog::showDialog>()
		);
	
		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/map", 	// menu location path
				"DifficultyEditor", // name
				ui::menuItem,	// type
				"Difficulty...",	// caption
				"stimresponse.png",	// icon
				"DifficultyEditor"); // event name
	}
};
typedef boost::shared_ptr<DifficultyEditorModule> DifficultyEditorModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(DifficultyEditorModulePtr(new DifficultyEditorModule));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getErrorStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
