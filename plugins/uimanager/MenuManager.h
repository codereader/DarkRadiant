#ifndef MENUMANAGER_H_
#define MENUMANAGER_H_

#include <string>
#include "iuimanager.h"
#include "MenuItem.h"

// Forward declarations
typedef struct _GtkWidget GtkWidget;

/** greebo: The MenuManager takes care of adding and inserting the 
 * 			menuitems at the given paths.
 * 
 * A valid menupath is for example: "main/file/exit" and consists of:
 * 
 * <menubarID>/<menuItem>/.../<menuItem>
 * 
 * The first part of the path is the name of the menubar you want to access.
 * (the MenuManager can of course keep track of several menubars).
 *  
 * Use the add() and insert() commands to create menuitems, the GtkWidget*
 * pointers are delivered as return value.
 * 
 */
namespace ui {

class MenuManager :
	public IMenuManager
{
	// The root item containing the menubars as children
	MenuItemPtr _root;	

public:
	// Constructor, initialises the menu from the registry
	MenuManager();
	
	/** greebo: Retrieves the menuitem widget specified by the path.
	 * 
	 * Example: get("main/file/open") delivers the widget for the "Open..." command.
	 * 
	 * @returns: the widget, or NULL, if no the path hasn't been found.
	 */
	GtkWidget* get(const std::string& path);
	
	/** greebo: Shows/hides the menuitem under the given path. 
	 * 
	 * @path: the path to the item (e.g. "main/view/cameraview")
	 * @visible: FALSE, if the widget should be hidden, TRUE otherwise
	 */
	void setVisibility(const std::string& path, bool visible);
	
	/** greebo: Adds a new item as child under the given path.
	 * 
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new item
	 * @type: the item type (usually menuFolder / menuItem)
	 * @caption: the display string of the menu item (incl. mnemonic)
	 * @icon: the icon filename (can be empty)
	 * @eventname: the event name (e.g. "ToggleShowSizeInfo")
	 */
	GtkWidget* add(const std::string& insertPath,
				   const std::string& name,
			 	   eMenuItemType type, 
				   const std::string& caption,
				   const std::string& icon,
				   const std::string& eventName);
	
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
	GtkWidget* insert(const std::string& insertPath,
					  const std::string& name,
					  eMenuItemType type,
					  const std::string& caption,
					  const std::string& icon,
					  const std::string& eventName);
	
	/** greebo: Loads all the menu items from the registry, called upon initialisation.
	 */		 
	void loadFromRegistry();
	
	/** greebo: Recursively go through the menus and update all the accelerator 
	 *          strings of all the menu items.
	 */
	void updateAccelerators();
};

} // namespace ui

#endif /*MENUMANAGER_H_*/
