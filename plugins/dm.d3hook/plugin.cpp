#include "imodule.h"

#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "iradiant.h"

#include "stream/stringstream.h"
#include "stream/textstream.h"
#include "generic/callback.h"

#include "DarkModCommands.h"

void CompileMap() {
	// Prevent re-entering this method
	static bool mutex(false);
	
	if (mutex) {
		return;
	}
	
	mutex = true;
	
	// Compile including AAS
	darkmodcommands::CompileMap(false);
	
	mutex = false;
}

void CompileMapNoAAS() {
	// Prevent re-entering this method
	static bool mutex(false);
	
	if (mutex) {
		return;
	}
	
	mutex = true;
	
	// Compile excluding AAS
	darkmodcommands::CompileMap(true);
	
	mutex = false;
}

/**
 * Module to register the menu commands for the D3Hook class.
 */
class D3HookModule : 
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("Doom3Hook");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_RADIANT);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << getName().c_str() << "::initialiseModule called.\n";
		
		// Add the callback event
		GlobalEventManager().addCommand("CompileMap", FreeCaller<CompileMap>());
		GlobalEventManager().addCommand("CompileMapNoAAS", FreeCaller<CompileMapNoAAS>());
	
		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/map", 	// menu location path
				"compilemap", // name
				ui::menuItem,	// type
				"Compile Map",	// caption
				"icon_classname.png",	// icon
				"CompileMap"); // event name
		
		mm.add("main/map", 	// menu location path
				"compilemapnoaas", // name
				ui::menuItem,	// type
				"Compile Map (no AAS)",	// caption
				"icon_classname.png",	// icon
				"CompileMapNoAAS"); // event name
	}
};
typedef boost::shared_ptr<D3HookModule> D3HookModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(D3HookModulePtr(new D3HookModule));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getErrorStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
