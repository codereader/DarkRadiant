#include "imodule.h"

#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "iradiant.h"

#include "stream/stringstream.h"
#include "stream/textstream.h"
#include "generic/callback.h"

#include "DarkModCommands.h"
#include "DarkRadiantRCFServer.h"

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
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << getName().c_str() << "::initialiseModule called.\n";
		
		if (GlobalRegistry().findXPath(RKEY_D3HOOK_CONSOLE_ENABLED).size() == 0) {
			// Write the default setting to the Registry
			GlobalRegistry().set(RKEY_D3HOOK_CONSOLE_ENABLED, "1");
		}
		
		// Add the callback event
		GlobalEventManager().addCommand(
			"MapCompile", FreeCaller<DarkModCommands::compileMap>()
		);
		GlobalEventManager().addCommand(
			"MapCompileNoAAS", FreeCaller<DarkModCommands::compileMapNoAAS>()
		);
		GlobalEventManager().addCommand(
			"MapRunAAS", FreeCaller<DarkModCommands::runAAS>()
		);
		GlobalEventManager().addRegistryToggle(
			"D3HookEnableConsole", RKEY_D3HOOK_CONSOLE_ENABLED
		);
		
		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/map", 	// menu location path
				"compilemap", // name
				ui::menuItem,	// type
				"Compile Map",	// caption
				"compile.png",	// icon
				"MapCompile"); // event name
		
		mm.add("main/map", 	// menu location path
				"compilemapnoaas", // name
				ui::menuItem,	// type
				"Compile Map (no AAS)",	// caption
				"compile.png",	// icon
				"MapCompileNoAAS"); // event name
		
		mm.add("main/map", 	// menu location path
				"runaas", // name
				ui::menuItem,	// type
				"Compile AAS only",	// caption
				"compile.png",	// icon
				"MapRunAAS"); // event name
	
		mm.add("main/map", 	// menu location path
				"enableconsoleoutput", // name
				ui::menuItem,	// type
				"Enable Console Output",	// caption
				"",	// icon
				"D3HookEnableConsole"); // event name
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
