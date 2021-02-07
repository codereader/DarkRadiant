#include "UIManager.h"
#include "module/StaticModule.h"

#include "itextstream.h"
#include "imainframe.h"
#include "GroupDialog.h"

namespace ui
{

IGroupDialog& UIManager::getGroupDialog() {
	return GroupDialog::Instance();
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
}

module::StaticModule<UIManager> uiManagerModule;

} // namespace ui
