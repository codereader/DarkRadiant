#include "UIManager.h"
#include "module/StaticModule.h"

#include "i18n.h"
#include "itextstream.h"
#include "iregistry.h"
#include "iradiant.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "GroupDialog.h"
#include "debugging/debugging.h"
#include "wxutil/dialog/MessageBox.h"

#include "LocalBitmapArtProvider.h"

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <memory>

namespace ui
{

IDialogManager& UIManager::getDialogManager()
{
	return *_dialogManager;
}

IMenuManager& UIManager::getMenuManager() {
	return *_menuManager;
}

IGroupDialog& UIManager::getGroupDialog() {
	return GroupDialog::Instance();
}

void UIManager::clear()
{
	_menuManager->clear();
	_dialogManager.reset();

	wxFileSystem::CleanUpHandlers();
	wxArtProvider::Delete(_bitmapArtProvider);
	_bitmapArtProvider = nullptr;
}

const std::string& UIManager::ArtIdPrefix() const
{
	return LocalBitmapArtProvider::ArtIdPrefix();
}

const std::string& UIManager::getName() const
{
	static std::string _name(MODULE_UIMANAGER);
	return _name;
}

const StringSet& UIManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void UIManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;

	_bitmapArtProvider = new LocalBitmapArtProvider();
	wxArtProvider::Push(_bitmapArtProvider);

	_dialogManager = std::make_shared<DialogManager>();

    _menuManager = std::make_shared<MenuManager>();
	_menuManager->loadFromRegistry();

	GlobalMainFrame().signal_MainFrameShuttingDown().connect(
        sigc::mem_fun(this, &UIManager::clear)
    );

	wxFileSystem::AddHandler(new wxLocalFSHandler);
	wxXmlResource::Get()->InitAllHandlers();

	std::string fullPath = ctx.getRuntimeDataPath() + "ui/";
	wxXmlResource::Get()->Load(fullPath + "*.xrc");
}

void UIManager::shutdownModule()
{
	_menuManager->clear();
}

module::StaticModule<UIManager> uiManagerModule;

} // namespace ui
