#include "UIManager.h"

#include "itextstream.h"
#include "iregistry.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "generic/callback.h"
#include "colourscheme/ColourSchemeEditor.h"
#include "GroupDialog.h"
#include "ShutdownListener.h"
#include "debugging/debugging.h"

namespace ui {

IDialogManager& UIManager::getDialogManager()
{
	return *_dialogManager;
}

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

IStatusBarManager& UIManager::getStatusBarManager() {
	return _statusBarManager;
}

void UIManager::clear()
{
	_menuManager.clear();
	_dialogManager = DialogManagerPtr();
}

const std::string& UIManager::getName() const {
	static std::string _name(MODULE_UIMANAGER);
	return _name;
}

const StringSet& UIManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_RADIANT);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void UIManager::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << "UIManager::initialiseModule called" << std::endl;

	_dialogManager = DialogManagerPtr(new DialogManager);

	_menuManager.loadFromRegistry();
	_toolbarManager.initialise();
	ColourSchemeManager::Instance().loadColourSchemes();
	
	GlobalCommandSystem().addCommand("EditColourScheme", ColourSchemeEditor::editColourSchemes);
	GlobalEventManager().addCommand("EditColourScheme", "EditColourScheme");

	_shutdownListener = UIManagerShutdownListenerPtr(new UIManagerShutdownListener(*this));
	GlobalRadiant().addEventListener(_shutdownListener);
}

} // namespace ui

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(ui::UIManagerPtr(new ui::UIManager));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
