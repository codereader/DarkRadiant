#include "imodule.h"

#include "ieventmanager.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include "iuimanager.h"
#include "iregistry.h"
#include "iselection.h"
#include "iradiant.h"
#include "iundo.h"

#include "scenelib.h"
#include "generic/callback.h"
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
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_RADIANT);
			_dependencies.insert(MODULE_SELECTIONSYSTEM);
			_dependencies.insert(MODULE_SCENEGRAPH);
			_dependencies.insert(MODULE_ECLASSMANAGER);
			_dependencies.insert(MODULE_UNDOSYSTEM);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "StimResponseModule::initialiseModule called.\n";
		
		// Add the callback event
		GlobalEventManager().addCommand(
			"StimResponseEditor", 
			FreeCaller<ui::StimResponseEditor::toggle>()
		);
	
		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/entity", 	// menu location path
				"StimResponse", // name
				ui::menuItem,	// type
				"Stim/Response...",	// caption
				"stimresponse.png",	// icon
				"StimResponseEditor"); // event name
	}

	virtual void shutdownModule() {
		// Clear the shared_ptr, in case it holds an instance
		ui::StimResponseEditor::destroyInstance();
	}
};
typedef boost::shared_ptr<StimResponseModule> StimResponseModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(StimResponseModulePtr(new StimResponseModule));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getErrorStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
