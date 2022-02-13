#include "GroupDialogManager.h"
#include "module/StaticModule.h"

#include "itextstream.h"
#include "GroupDialog.h"

namespace ui
{

IGroupDialog& GroupDialogManager::get()
{
	return GroupDialog::Instance();
}

const std::string& GroupDialogManager::getName() const
{
	static std::string _name(MODULE_GROUPDIALOG);
	return _name;
}

const StringSet& GroupDialogManager::getDependencies() const
{
	static StringSet _dependencies;
	return _dependencies;
}

void GroupDialogManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;
}

module::StaticModuleRegistration<GroupDialogManager> groupDialogManagerModule;

} // namespace ui
