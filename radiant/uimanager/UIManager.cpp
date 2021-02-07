#include "UIManager.h"
#include "module/StaticModule.h"

#include "itextstream.h"
#include "imainframe.h"
#include "GroupDialog.h"

#include <wx/xrc/xmlres.h>

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
		_dependencies.insert(MODULE_MAINFRAME);
	}

	return _dependencies;
}

void UIManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;

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
