#pragma once

#include <string>
#include "iuimanager.h"

/** 
 * greebo: The MenuManager takes care of adding and inserting the
 * menuitems at the given paths.
 *
 * A valid menupath is for example: "main/file/exit" and consists of:
 *
 * <menubarID>/<menuItem>/.../<menuItem>
 *
 * The first part of the path is the name of the menubar you want to access.
 * (the MenuManager can of course keep track of several menubars).
 *
 * Use the add() and insert() commands to create menuitems.
 */
namespace ui
{

class MenuElement;
typedef std::shared_ptr<MenuElement> MenuElementPtr;

class MenuManager :
	public IMenuManager
{
private:
	// All menubars are child of this root item
	MenuElementPtr _root;

public:
	// Constructor, initialises the menu from the registry
	MenuManager();

	wxMenuBar* getMenuBar(const std::string& name) override;

	void setVisibility(const std::string& path, bool visible) override;

	/** greebo: Adds a new item as child under the given path.
	 *
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new item
	 * @type: the item type (usually menuFolder / menuItem)
	 * @caption: the display string of the menu item (incl. mnemonic)
	 * @icon: the icon filename (can be empty)
	 * @eventname: the event name (e.g. "ToggleShowSizeInfo")
	 */
	void add(const std::string& insertPath,
			 const std::string& name,
			 eMenuItemType type,
			 const std::string& caption,
			 const std::string& icon,
			 const std::string& eventName) override;

	/** greebo: Inserts a new menuItem as sibling _before_ the given insertPath.
	 *
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new menu item (no path, just the name)
	 * @caption: the display string including mnemonic
	 * @icon: the image file name relative to "bitmaps/", can be empty.
	 * @eventName: the event name this item is associated with (can be empty).
	 */
	void insert(const std::string& insertPath,
				const std::string& name,
				eMenuItemType type,
				const std::string& caption,
				const std::string& icon,
				const std::string& eventName) override;

	bool exists(const std::string& path) override;

	/**
	 * Removes an entire menu subtree.
	 */
	void remove(const std::string& path) override;

	/** greebo: Loads all the menu items from the registry, called upon initialisation.
	 */
	void loadFromRegistry();

	/**
	 * Clears all references to widgets etc.
	 */
	void clear();

private:
	void handleElementAdded(const MenuElementPtr& element);
	void handleElementRemoved(const MenuElementPtr& element);

	// Returns the top level menu (== MenuFolder) this element is part of
	MenuElementPtr findTopLevelMenu(const MenuElementPtr& element);
};

} // namespace ui
