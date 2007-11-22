#include "imodule.h"

#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "iradiant.h"
#include "iscenegraph.h"
#include "nameable.h"

#include "stream/stringstream.h"
#include "stream/textstream.h"
#include "generic/callback.h"

#include "os/path.h"

#include "DarkModRCFClient.h"

void CompileMap() {
	// Prevent re-entering this method
	static bool mutex(false);
	
	if (mutex) {
		return;
	}
	
	mutex = true;
	
	// Add the menu item
	IMenuManager& mm = GlobalUIManager().getMenuManager();
	// TODO: Set sensitivity of menu item during compilation to FALSE
	/*mm.add("main/map", 	// menu location path
			"compilemap", // name
			ui::menuItem,	// type
			"Compile Map (dmap)",	// caption
			"icon_classname.png",	// icon
			"CompileMap"); // event name*/

	// Get the map name
	// Note: this is a temporary way to retrieve the map name 
	// until a proper IMap interface is in place
	NameablePtr rootNameable = 
		boost::dynamic_pointer_cast<Nameable>(GlobalSceneGraph().root());

	if (rootNameable != NULL && rootNameable->name().size() > 0) {
		std::string fullName(rootNameable->name());

		if (boost::algorithm::istarts_with(fullName, "maps/")) {
			fullName = fullName.substr(5, fullName.size());
		}

		// Instantiate a client and issue the command
		DarkModRCFClient client;
		client.executeCommand("dmap " + fullName);
	}
	
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
		GlobalEventManager().addCommand(
			"CompileMap", 
			FreeCaller<CompileMap>()
		);
	
		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/map", 	// menu location path
				"compilemap", // name
				ui::menuItem,	// type
				"Compile Map (dmap)",	// caption
				"icon_classname.png",	// icon
				"CompileMap"); // event name
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
