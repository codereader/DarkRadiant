#pragma once

#include "imodule.h"

// Forward declarations
class IGroupDialog;		// see igroupdialog.h for definition

const char* const MODULE_UIMANAGER("UIManager");

/** 
 * greebo: The UI Manager gives access to the GroupDialog and the DialogManager.
 */
class IUIManager :
	public RegisterableModule
{
public:
	virtual IGroupDialog& getGroupDialog() = 0;
};

inline IUIManager& GlobalUIManager()
{
	static module::InstanceReference<IUIManager> _reference(MODULE_UIMANAGER);
	return _reference;
}

inline IGroupDialog& GlobalGroupDialog()
{
	return GlobalUIManager().getGroupDialog();
}
