#pragma once

#include "imodule.h"
#include "igroupdialog.h"

namespace ui
{

/** 
 * greebo: The UI Manager gives access to the GroupDialog and the DialogManager.
 */
class IGroupDialogManager :
	public RegisterableModule
{
public:
    virtual ~IGroupDialogManager() {}

	virtual IGroupDialog& get() = 0;
};

}

constexpr const char* const MODULE_GROUPDIALOG("GroupDialogModule");

inline ui::IGroupDialogManager& GlobalGroupDialogManager()
{
	static module::InstanceReference<ui::IGroupDialogManager> _reference(MODULE_GROUPDIALOG);
	return _reference;
}

inline IGroupDialog& GlobalGroupDialog()
{
	return GlobalGroupDialogManager().get();
}
