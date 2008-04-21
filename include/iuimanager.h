#ifndef INCLUDE_UIMANAGER_H_
#define INCLUDE_UIMANAGER_H_

#include "math/Vector3.h"
#include "imodule.h"

// Forward declarations
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkToolbar GtkToolbar;

class IColourSchemeManager {
public:
	// greebo: Returns the named colour, returns <0,0,0> if not found
	virtual Vector3 getColour(const std::string& colourName) = 0;
};

namespace ui {
	/** greebo: The possible menu item types, one of these
	 * 			has to be passed when creating menu items.
	 */
	enum eMenuItemType {
		menuNothing,
		menuRoot,
		menuBar,
		menuFolder,
		menuItem,
		menuSeparator,
	};
} // namespace ui

/** greebo: Implementation documentation: see MenuManager.h.
 */
class IMenuManager
{
public:
	/** greebo: Retrieves the menuitem widget specified by the path.
	 * 
	 * Example: get("main/file/open") delivers the widget for the "Open..." command.
	 * 
	 * @returns: the widget, or NULL, if no the path hasn't been found.
	 */
	virtual GtkWidget* get(const std::string& path) = 0;
	
	/** greebo: Shows/hides the menuitem under the given path. 
	 * 
	 * @path: the path to the item (e.g. "main/view/cameraview")
	 * @visible: FALSE, if the widget should be hidden, TRUE otherwise
	 */
	virtual void setVisibility(const std::string& path, bool visible) = 0;
	
	/** greebo: Adds a new item as child under the given path.
	 * 
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new item
	 * @type: the item type (usually menuFolder / menuItem)
	 * @caption: the display string of the menu item (incl. mnemonic)
	 * @icon: the icon filename (can be empty)
	 * @eventname: the event name (e.g. "ToggleShowSizeInfo")
	 */
	virtual GtkWidget* add(const std::string& insertPath,
						   const std::string& name,
						   ui::eMenuItemType type, 
						   const std::string& caption, 
						   const std::string& icon,
						   const std::string& eventName) = 0;
			 
	/** greebo: Inserts a new menuItem as sibling _before_ the given insertPath.
	 * 
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new menu item (no path, just the name)
	 * @caption: the display string including mnemonic
	 * @icon: the image file name relative to "bitmaps/", can be empty.
	 * @eventName: the event name this item is associated with (can be empty).
	 * 
	 * @returns: the GtkWidget* 
	 */
	virtual GtkWidget* insert(const std::string& insertPath,
							  const std::string& name,
							  ui::eMenuItemType type,
							  const std::string& caption,
							  const std::string& icon,
							  const std::string& eventName) = 0;
	
	/** Recursively iterates over the menu items and updates
	 *  the accelerator strings by requesting them from the EventManager.
	 */
	virtual void updateAccelerators() = 0;
};

class IToolbarManager
{
public:
	virtual GtkToolbar* getToolbar(const std::string& toolbarName) = 0;
};

// Forward declaration, see igroupdialog.h for definition
class IGroupDialog;

const std::string MODULE_UIMANAGER("UIManager");

/** greebo: The UI Manager abstract base class.
 * 
 * The UIManager provides an interface to add UI items like menu commands
 * toolbar icons, update status bar texts and such. 
 */
class IUIManager :
	public RegisterableModule
{
public:
	virtual IMenuManager& getMenuManager() = 0;
	virtual IToolbarManager& getToolbarManager() = 0;
	virtual IColourSchemeManager& getColourSchemeManager() = 0;
	virtual IGroupDialog& getGroupDialog() = 0;
};

// This is the accessor for the UI manager
inline IUIManager& GlobalUIManager() {
	// Cache the reference locally
	static IUIManager& _uiManager(
		*boost::static_pointer_cast<IUIManager>(
			module::GlobalModuleRegistry().getModule(MODULE_UIMANAGER)
		)
	);
	return _uiManager;
}

// Shortcut accessors
inline IColourSchemeManager& ColourSchemes() {
	return GlobalUIManager().getColourSchemeManager();
}

inline IGroupDialog& GlobalGroupDialog() {
	return GlobalUIManager().getGroupDialog();
}

#endif /*INCLUDE_UIMANAGER_H_*/
