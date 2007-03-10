#ifndef MENUITEM_H_
#define MENUITEM_H_

#include "ieventmanager.h"
#include "xmlutil/Node.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace ui {

class MenuItem;
typedef boost::shared_ptr<MenuItem> MenuItemPtr;
typedef std::vector<MenuItemPtr> MenuItemList;

class MenuItem
{
public:
	enum eType {
		eNothing,
		eRoot,
		eFolder,
		eItem,
		eSeparator,
	};

private:	
	// The parent of this MenuItem
	MenuItemPtr _parent;
	
	// The name of this node
	std::string _name;
	
	// The caption (display string) incl. the mnemonic
	std::string _caption;
	
	// The associated event
	IEventPtr _event;
	
	// The associated GtkWidget
	GtkWidget* _widget;
	
	// The children of this MenuItem
	MenuItemList _children;
	
	eType _type;
	
public:
	// Constructor, needs a name and a parent specified
	MenuItem(MenuItemPtr parent);

	// The name of this MenuItem
	std::string getName() const;
	void setName(const std::string& name);
	
	// Returns TRUE if this item has no parent item
	bool isRoot() const;
	
	// Returns the pointer to the parent (is NULL for the root item)
	MenuItemPtr getParent() const;
	
	// Returns the type of this item node
	eType getType() const;
	
	// Gets/sets the caption of this item
	void setCaption(const std::string& caption);
	std::string getCaption() const;
	
	// Returns TRUE if this has no actual event assigned
	bool isEmpty() const;
	
	// Returns the number of child items
	int numChildren() const;
	
	// Return the event (is NULL for an empty item)
	IEventPtr getEvent() const;

	// Sets the event of this item by defining the event name
	void setEvent(const std::string& eventName);
	
	// Use this to instantiate the Gtk menu widget out of this item
	operator GtkWidget* ();
	
	// Tries to (recursively) locate the menuitem by looking up the path 
	MenuItemPtr find(const std::string& menuPath);
	
	/** greebo: Takes the given xml::Node and populates this item recursively.
	 * 			Pass the MenuItemPtr to this object when calling it, so that 
	 * 			the item can pass it to its children.
	 * 			At least I don't know of any other method right now to 
	 * 			create a "self" shared_ptr from itself.
	 */
	void parseNode(xml::Node& node, MenuItemPtr thisItem);
};

} // namespace ui

#endif /*MENUITEM_H_*/
