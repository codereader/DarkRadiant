#include "UIManager.h"

#include "iregistry.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "stream/textstream.h"
#include "generic/callback.h"
#include "colourscheme/ColourSchemeEditor.h"
#include "GroupDialog.h"

namespace ui {

IMenuManager& UIManager::getMenuManager() {
	return _menuManager;
}

IToolbarManager& UIManager::getToolbarManager() {
	return _toolbarManager;
}

IColourSchemeManager& UIManager::getColourSchemeManager() {
	return ColourSchemeManager::Instance();
}

IGroupDialog& UIManager::getGroupDialog() {
	return GroupDialog::Instance();
}

const std::string& UIManager::getName() const {
	static std::string _name(MODULE_UIMANAGER);
	return _name;
}

const StringSet& UIManager::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_RADIANT);
	}

	return _dependencies;
}

void UIManager::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "UIManager::initialiseModule called\n";
	_menuManager.loadFromRegistry();
	_toolbarManager.initialise();
	ColourSchemeManager::Instance().loadColourSchemes();
	
	GlobalEventManager().addCommand(
		"EditColourScheme", 
		FreeCaller<ColourSchemeEditor::editColourSchemes>()
	);
}

} // namespace ui

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(ui::UIManagerPtr(new ui::UIManager));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
