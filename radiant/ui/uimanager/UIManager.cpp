#include "UIManager.h"

#include "iregistry.h"
#include "ieventmanager.h"
#include <boost/shared_ptr.hpp>

namespace ui {

	namespace {
		
	}

UIManager::UIManager() {
	
}

void UIManager::addMenuItem(const std::string& menuPath, 
							const std::string& caption, 
					 		const std::string& eventName) 
{
	_menu.add(menuPath, caption, eventName);
}

} // namespace ui

// Module stuff

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class UIManagerDependencies :
	public GlobalEventManagerModuleRef,
	public GlobalRegistryModuleRef
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
	}
	
	IUIManager* getTable() {
		return _uiManager.get();
	}
};

typedef SingletonModule<UIManagerAPI, UIManagerDependencies> UIManagerModule;
typedef Static<UIManagerModule> StaticUIManagerModule;
StaticRegisterModule staticRegisterUIManager(StaticUIManagerModule::instance());
