#include "UIManager.h"

#include "ieventmanager.h"
#include <boost/shared_ptr.hpp>

namespace ui {


} // namespace ui

// Module stuff

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class UIManagerDependencies :
	public GlobalEventManagerModuleRef
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
