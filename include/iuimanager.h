#ifndef INCLUDE_UIMANAGER_H_
#define INCLUDE_UIMANAGER_H_

#include <string>
#include "generic/constant.h"

// Forward declarations
typedef struct _GtkWidget GtkWidget;

namespace ui {
	enum eMenuItemType {
		menuNothing,
		menuRoot,
		menuBar,
		menuFolder,
		menuItem,
		menuSeparator,
	};
}

class IMenuManager
{
public:
	/** greebo: Retrieves the menu with the given <name>.
	 * 
	 * @returns: the widget, or NULL, if no <name> menu has been found.
	 */
	virtual GtkWidget* getMenu(const std::string& name) = 0;
	
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
};

/** greebo: The UI Manager abstract base class.
 * 
 * The UIManager provides an interface to add UI items like menu commands
 * toolbar icons, update status bar texts and such. 
 */
class IUIManager
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "UIManager");

	virtual IMenuManager* getMenuManager() = 0;
};

// Module definitions

#include "modulesystem.h"

template<typename Type>
class GlobalUIModule;
typedef GlobalModule<IUIManager> GlobalUIManagerModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IUIManager> GlobalUIManagerModuleRef;

// This is the accessor for the event manager
inline IUIManager& GlobalUIManager() {
	return GlobalUIManagerModule::getTable();
}

#endif /*INCLUDE_UIMANAGER_H_*/
