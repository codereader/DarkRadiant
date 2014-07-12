#include "UIManager.h"

#include "itextstream.h"
#include "iregistry.h"
#include "iradiant.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "colourscheme/ColourSchemeEditor.h"
#include "GroupDialog.h"
#include "debugging/debugging.h"
#include "FilterMenu.h"
#include "wxutil/dialog/MessageBox.h"

#include "LocalBitmapArtProvider.h"

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>

namespace ui
{

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

IFilterMenuPtr UIManager::createFilterMenu()
{
	return IFilterMenuPtr(new FilterMenu);
}

void UIManager::clear()
{
	_statusBarManager.onRadiantShutdown();

	_menuManager.clear();
	_dialogManager = DialogManagerPtr();

	wxFileSystem::CleanUpHandlers();
	wxArtProvider::Delete(_bitmapArtProvider);
	_bitmapArtProvider = NULL;
}

const std::string& UIManager::ArtIdPrefix() const
{
	return LocalBitmapArtProvider::ArtIdPrefix();
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
	rMessage() << "UIManager::initialiseModule called" << std::endl;

	_bitmapArtProvider = new LocalBitmapArtProvider();
	wxArtProvider::Push(_bitmapArtProvider);

	_dialogManager = DialogManagerPtr(new DialogManager);

	_menuManager.loadFromRegistry();
	_toolbarManager.initialise();
	ColourSchemeManager::Instance().loadColourSchemes();

	GlobalCommandSystem().addCommand("EditColourScheme", ColourSchemeEditor::DisplayDialog);
	GlobalEventManager().addCommand("EditColourScheme", "EditColourScheme");

	GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(this, &UIManager::clear)
    );

	// Add the statusbar command text item
	_statusBarManager.addTextElement(
		STATUSBAR_COMMAND,
		"",  // no icon
		IStatusBarManager::POS_COMMAND
	);

	wxFileSystem::AddHandler(new wxLocalFSHandler);
	wxXmlResource::Get()->InitAllHandlers();

	std::string fullPath = ctx.getRuntimeDataPath() + "ui/";
	wxXmlResource::Get()->Load(fullPath + "*.xrc");
}

void UIManager::shutdownModule()
{
}

} // namespace ui

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	registry.registerModule(ui::UIManagerPtr(new ui::UIManager));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
