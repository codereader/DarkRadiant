#include "imodule.h"

#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "iradiant.h"
#include "ieclass.h"

#include "stream/textstream.h"
#include "generic/callback.h"

#include "EClassTree.h"

/**
 * Module to register the menu commands for the EntityClassTree class.
 */
class EClassTreeModule : 
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("EClassTree");
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
			"EntityClassTree", 
			FreeCaller<ui::EClassTree::showWindow>()
		);
	
		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/entity", 	// menu location path
				"EntityClassTree", // name
				ui::menuItem,	// type
				"Entity Class Tree...",	// caption
				"icon_classname.png",	// icon
				"EntityClassTree"); // event name
	}
};
typedef boost::shared_ptr<EClassTreeModule> EClassTreeModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(EClassTreeModulePtr(new EClassTreeModule));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
