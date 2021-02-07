#pragma once

#include "imodule.h"

// Forward declarations
class wxWindow;

// Forward declarations
class IGroupDialog;		// see igroupdialog.h for definition

namespace ui
{

class IDialogManager;	// see idialogmanager.h for definition

} // namespace ui

const char* const MODULE_UIMANAGER("UIManager");

/** greebo: The UI Manager abstract base class.
 *
 * The UIManager provides an interface to add UI items 
 * like menu commands.
 */
class IUIManager :
	public RegisterableModule
{
public:
	virtual IGroupDialog& getGroupDialog() = 0;
	virtual ui::IDialogManager& getDialogManager() = 0;

	// Returns the art provider prefix to acquire local bitmaps from the wxWidgets art provider
	// Example: wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "darkradiant_icon_64x64.png")
	virtual const std::string& ArtIdPrefix() const = 0;
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
