#pragma once

#include "imodule.h"

// Forward declarations
class IGroupDialog;		// see igroupdialog.h for definition

namespace ui
{

class IDialogManager;	// see idialogmanager.h for definition

} // namespace ui

const char* const MODULE_UIMANAGER("UIManager");

/** 
 * greebo: The UI Manager gives access to the GroupDialog and the DialogManager.
 */
class IUIManager :
	public RegisterableModule
{
public:
	virtual IGroupDialog& getGroupDialog() = 0;
	virtual ui::IDialogManager& getDialogManager() = 0;
};

// This is the accessor for the UI manager
inline IUIManager& GlobalUIManager()
{
	static module::InstanceReference<IUIManager> _reference(MODULE_UIMANAGER);
	return _reference;
}

inline IGroupDialog& GlobalGroupDialog() {
	return GlobalUIManager().getGroupDialog();
}
