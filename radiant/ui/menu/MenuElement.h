#pragma once

#include "xmlutil/Node.h"
#include "ui/imenumanager.h"
#include <vector>
#include <memory>

/** greebo: This is a representation of a general menu item/element.
 *
 * The possible menuitem types are defined in imenumanager.h.
 * Each menu item can have a list of sub-MenuElements (this applies to the
 * types eMenuBar and eFolder).
 *
 * Use the MenuManager class to access these MenuElements.
 */
namespace ui
{

namespace menu
{

class MenuElement;
typedef std::shared_ptr<MenuElement> MenuElementPtr;
typedef std::weak_ptr<MenuElement> MenuElementWeakPtr;

class MenuElement :
	public IMenuElement,
	public std::enable_shared_from_this<MenuElement>
{
protected:
	// The parent of this MenuElement (weak reference to avoid circular ownership)
	MenuElementWeakPtr _parent;

	// The name of this node
	std::string _name;

	// The caption (display string) incl. the mnemonic
	std::string _caption;

	// The icon name
	std::string _icon;

	// The associated event name
	std::string _event;

	// The children of this MenuElement
	typedef std::vector<MenuElementPtr> MenuElementList;
	MenuElementList _children;

	bool _isVisible;

	// Checked if anything in this item or below has changed
	// and needs reconstruction the next time the menu is opened
	bool _needsRefresh;

	static int _nextMenuItemId;

public:
	// Constructor, needs a name and a parent specified
	MenuElement(const MenuElementPtr& parent = MenuElementPtr());

	// Destructor disconnects the widget from the event
	~MenuElement();

	// The name of this MenuElement
	const std::string& getName() const override;
	void setName(const std::string& name);

	void setIcon(const std::string& icon);

	// Returns the pointer to the parent (is NULL for the root item)
	MenuElementPtr getParent() const;
	void setParent(const MenuElementPtr& parent);

	bool isVisible() const;
	void setIsVisible(bool visible);

	/**
	 * greebo: Adds the given MenuElement at the end of the child list.
	 */
	void addChild(const MenuElementPtr& newChild);

	/**
	* greebo: Adds the given MenuElement to the list of children.
	* @pos: the element this element is inserted before.
	*/
	void insertChild(const MenuElementPtr& newChild, const MenuElementPtr& insertBefore);

	/**
	 * Removes the given child from this menu item.
	 */
	void removeChild(const MenuElementPtr& child);

	// Removes all child nodes
	void removeAllChildren();

	/** 
	 * greebo: Returns the index/position of the given child. Hidden elements do not count.
	 */
	int getMenuPosition(const MenuElementPtr& child, bool includeHidden = false);

	// Gets/sets the caption of this item
	void setCaption(const std::string& caption);
	std::string getCaption() const;

	// Returns the number of child items
	std::size_t numChildren() const;

	// Return / set the event name
	const std::string& getEvent() const override;
	void setEvent(const std::string& eventName);

	bool needsRefresh();
	virtual void setNeedsRefresh(bool needsRefresh);

	virtual void setAccelerator(const std::string& accelStr) override;

	virtual bool isToggle() const override
	{
		return false;
	}

	virtual void setToggled(bool isToggled) override
	{}

	// Tries to (recursively) locate the MenuElement by looking up the path
	MenuElementPtr find(const std::string& menuPath);

    /**
     * @brief Parses the given XML node recursively and creates all items from the
     * information it finds.
     * 
     * @param node 
     * XML node containing the menu information.
     *
     * @return Constructed MenuElement pointer or null if the menu item is not required in
     * the current game configuration.
     */
    static MenuElementPtr CreateFromNode(const xml::Node& node);

	/**
	 * Constructs a menu element for the given type
	 */
	static MenuElementPtr CreateForType(ItemType type);

protected:
	void setNeedsRefreshRecursively(bool needsRefresh);

	// Instantiates this all current child elements recursively as wxObjects
	// to be overridden by subclasses
	virtual void construct() = 0;

	// Destroys the wxWidget instantiation of this element and all children
	// (which also nullifies the widget pointers). Next time
	// the getWidget() method is called the widgets will be reconstructed.
	// Subclasses should override this
	virtual void deconstruct() = 0;

	// This default implementaton here does as exepected: constructs all children
	virtual void constructChildren();

	// This default implementation passes the deconstruct() call to all children
	virtual void deconstructChildren();
};

} // namespace

} // namespace
