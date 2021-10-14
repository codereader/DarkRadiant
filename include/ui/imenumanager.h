#pragma once

#include "imodule.h"

class wxMenuBar;

namespace ui
{

namespace menu
{

/** 
 * greebo: The possible menu item types.
 */
enum class ItemType
{
	Nothing,
	Root,
	Bar,
	Folder,
	Item,
	Separator,
};

// Interface of the menu items used by DarkRadiant's MenuManager
class IMenuElement
{
public:
	virtual ~IMenuElement() {}

	virtual ItemType getType() const = 0;

	// The name of this element
	virtual const std::string& getName() const = 0;

	// Returns the event/statement that is associated to this item
	virtual const std::string& getEvent() const = 0;

	virtual void setAccelerator(const std::string& accelStr) = 0;

	// Whether this item is a toggle-able item
	virtual bool isToggle() const = 0;

	virtual void setToggled(bool isToggled) = 0;
};

class IMenuManager :
    public RegisterableModule
{
public:
    /** Destructor
	 */
    virtual ~IMenuManager() {}

	/**
	 * Returns the constructed menu bar, ready for packing into a parent container.
	 */
	virtual wxMenuBar* getMenuBar(const std::string& name) = 0;

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
    virtual void add(const std::string& insertPath,
                     const std::string& name,
                     ItemType type,
                     const std::string& caption = "",
                     const std::string& icon = "",
                     const std::string& eventName = "") = 0;

	/** greebo: Inserts a new menuItem as sibling _before_ the given insertPath.
	 *
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new menu item (no path, just the name)
	 * @caption: the display string including mnemonic
	 * @icon: the image file name relative to "bitmaps/", can be empty.
	 * @eventName: the event name this item is associated with (can be empty).
	 */
	virtual void insert(const std::string& insertPath,
                        const std::string& name,
                        ItemType type,
                        const std::string& caption,
                        const std::string& icon,
                        const std::string& eventName) = 0;

	// Returns true if the given path exists
	virtual bool exists(const std::string& path) = 0;

	/**
	 * Removes an entire path from the menus.
 	 */
	virtual void remove(const std::string& path) = 0;
};

} // namespace

} // namespace

constexpr const char* const MODULE_MENUMANAGER = "MenuManager";

inline ui::menu::IMenuManager& GlobalMenuManager()
{
    static module::InstanceReference<ui::menu::IMenuManager> _reference(MODULE_MENUMANAGER);
    return _reference;
}
