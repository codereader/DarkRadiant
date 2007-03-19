#include "UIManager.h"

#include "iregistry.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "gtkutil/image.h"
#include <boost/shared_ptr.hpp>

namespace ui {

IMenuManager& UIManager::getMenuManager() {
	return _menuManager;
}

} // namespace ui

// Module stuff

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class UIManagerDependencies :
	public GlobalEventManagerModuleRef,
	public GlobalRegistryModuleRef,
	public GlobalRadiantModuleRef
{};

class UIManagerAPI
{
	typedef boost::shared_ptr<ui::UIManager> UIManagerPtr;
	UIManagerPtr _uiManager;

public:
	typedef IUIManager Type;
	STRING_CONSTANT(Name, "*");

	// Constructor
	UIManagerAPI() {
		// allocate a new UIManager instance on the heap (shared_ptr) 
		_uiManager = UIManagerPtr(new ui::UIManager);
		
		std::string bitmapsPath = GlobalRegistry().get("user/paths/bitmapsPath");
		BitmapsPath_set(bitmapsPath.c_str());
	}
	
	IUIManager* getTable() {
		return _uiManager.get();
	}
};

/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<UIManagerAPI, UIManagerDependencies> UIManagerModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server) {
	// Static instance of the BrushClipperModule
	static UIManagerModule _instance;
	initialiseModule(server);
	_instance.selfRegister();
}
