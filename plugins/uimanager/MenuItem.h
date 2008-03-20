#ifndef MENUITEM_H_
#define MENUITEM_H_

#include "xmlutil/Node.h"
#include "iuimanager.h"
#include "gtkutil/MenuItemAccelerator.h"
#include <vector>
#include <boost/shared_ptr.hpp>

typedef struct _GtkWidget GtkWidget;

/** greebo: This is a representation of a general menu item/element. 
 * 			
 * The possible menuitem types are defined in iuimanager.h.
 * Each menu item can have a list of sub-menuitems (this applies to the
 * types eMenuBar and eFolder).
 * 
 * Use the MenuManager class to access these menuitems. 
 */
namespace gtkutil {
	typedef boost::shared_ptr<TextMenuItemAccelerator> MenuItemAcceleratorPtr;
}

namespace ui {

class MenuItem;
typedef boost::shared_ptr<MenuItem> MenuItemPtr;
typedef std::vector<MenuItemPtr> MenuItemList;

class MenuItem
{
	// The parent of this MenuItem
	MenuItemPtr _parent;
	
	// The name of this node
	std::string _name;
	
	// The caption (display string) incl. the mnemonic
	std::string _caption;
	
	// The icon name
	std::string _icon;
	
	// The associated event
	std::string _event;
	
	// The associated GtkWidget
	gtkutil::MenuItemAcceleratorPtr _menuItem;
	
	GtkWidget* _widget;
	
	// The children of this MenuItem
	MenuItemList _children;
	
	eMenuItemType _type;
	
	// Stays false until the widgets are actually created.
	bool _constructed;
	
public:
	// Constructor, needs a name and a parent specified
	MenuItem(MenuItemPtr parent);

	// The name of this MenuItem
	std::string getName() const;
	void setName(const std::string& name);
	
	void setIcon(const std::string& icon);
	
	// Returns TRUE if this item has no parent item
	bool isRoot() const;
	
	// Returns the pointer to the parent (is NULL for the root item)
	MenuItemPtr parent() const;
	void setParent(MenuItemPtr parent);
	
	/** greebo: Adds the given menuitem to the list of children.
	 * 
	 *  Note: the new child is NOT reparented, the calling function must to this. 
	 */
	void addChild(MenuItemPtr newChild);
	
	/** greebo: Tries to find the GtkMenu position index of the given child.
	 */
	int getMenuPosition(MenuItemPtr child);
	
	// Returns the type of this item node
	eMenuItemType getType() const;
	void setType(eMenuItemType type);
	
	// Gets/sets the caption of this item
	void setCaption(const std::string& caption);
	std::string getCaption() const;
	
	// Returns TRUE if this has no actual event assigned
	bool isEmpty() const;
	
	// Returns the number of child items
	std::size_t numChildren() const;
	
	// Return / set the event name
	std::string getEvent() const;
	void setEvent(const std::string& eventName);
	
	// Use this to get the according Gtk menu widget out of this item.
	operator GtkWidget*();
	
	// Tries to (recursively) locate the menuitem by looking up the path 
	MenuItemPtr find(const std::string& menuPath);
	
	/** greebo: Takes the given xml::Node and populates this item recursively.
	 * 			Pass the MenuItemPtr to this object when calling it, so that 
	 * 			the item can pass it to its children.
	 * 			At least I don't know of any other method right now to 
	 * 			create a "self" shared_ptr from itself.
	 */
	void parseNode(xml::Node& node, MenuItemPtr thisItem);
	
	/** Updates the accelerator string of this item and all its children,
	 *  if applicable.
	 */
	void updateAcceleratorRecursive();
	
private:

	/** greebo: This constructs the actual widgets. This is invoked as soon
	 * 			as the first GtkWidget* cast of this object is requested.
	 */
	void construct();
};

} // namespace ui

#endif /*MENUITEM_H_*/
