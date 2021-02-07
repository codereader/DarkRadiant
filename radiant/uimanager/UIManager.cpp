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

#include "wxutil/LocalBitmapArtProvider.h"

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <memory>

namespace ui
{

IDialogManager& UIManager::getDialogManager()
{
	return *_dialogManager;
}

IGroupDialog& UIManager::getGroupDialog() {
	return GroupDialog::Instance();
}

void UIManager::clear()
{
	_dialogManager.reset();

	wxFileSystem::CleanUpHandlers();
	_bitmapArtProvider.reset();
}

const std::string& UIManager::ArtIdPrefix() const
{
	return wxutil::LocalBitmapArtProvider::ArtIdPrefix();
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

	_bitmapArtProvider.reset(new wxutil::LocalBitmapArtProvider(ctx.getBitmapsPath()));

	_dialogManager = std::make_shared<DialogManager>();

	GlobalMainFrame().signal_MainFrameShuttingDown().connect(
        sigc::mem_fun(this, &UIManager::clear)
    );

	wxFileSystem::AddHandler(new wxLocalFSHandler);
	wxXmlResource::Get()->InitAllHandlers();

	std::string fullPath = ctx.getRuntimeDataPath() + "ui/";
	wxXmlResource::Get()->Load(fullPath + "*.xrc");
}

module::StaticModule<UIManager> uiManagerModule;

} // namespace ui
