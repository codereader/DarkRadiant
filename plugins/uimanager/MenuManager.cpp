#include "MenuManager.h"

#include "itextstream.h"
#include "iregistry.h"

#include <gtkmm/widget.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/menushell.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

namespace ui {

	namespace {
		// The menu root key in the registry
		const std::string RKEY_MENU_ROOT = "user/ui/menu";
		const std::string TYPE_ITEM = "item";
		typedef std::vector<std::string> StringVector;
	}

MenuManager::MenuManager() :
	_root(new MenuItem(MenuItemPtr())) // Allocate the root item (type is set automatically)
{}

void MenuManager::clear() {
	_root = MenuItemPtr();
}

void MenuManager::loadFromRegistry() {
	xml::NodeList menuNodes = GlobalRegistry().findXPath(RKEY_MENU_ROOT);

	if (!menuNodes.empty()) {
		for (std::size_t i = 0; i < menuNodes.size(); i++) {
			std::string name = menuNodes[i].getAttributeValue("name");

			// Allocate a new MenuItem with root as parent
			MenuItemPtr menubar = MenuItemPtr(new MenuItem(_root));
			menubar->setName(name);

			// Populate the root menuitem using the current node
			menubar->parseNode(menuNodes[i], menubar);

			// Add the menubar as child of the root (child is already parented to _root)
			_root->addChild(menubar);
		}
	}
	else {
		rError() << "MenuManager: Could not find menu root in registry.\n";
	}
}

void MenuManager::setVisibility(const std::string& path, bool visible) {
	// Sanity check for empty menu
	if (_root == NULL) return;

	MenuItemPtr foundMenu = _root->find(path);

	if (foundMenu != NULL)
	{
		// Get the Widget* and set the visibility
		Gtk::Widget* menuitem = foundMenu->getWidget();

		if (visible)
		{
			menuitem->show();
		}
		else {
			menuitem->hide();
		}
	}
	else {
		rError() << "MenuManager: Warning: Menu " << path << " not found!\n";
	}
}

Gtk::Widget* MenuManager::get(const std::string& path) {
	// Sanity check for empty menu
	if (_root == NULL) return NULL;

	MenuItemPtr foundMenu = _root->find(path);

	if (foundMenu != NULL)
	{
		return foundMenu->getWidget();
	}
	else
	{
		//rError() << "MenuManager: Warning: Menu " << path.c_str() << " not found!\n";
		return NULL;
	}
}

Gtk::Widget* MenuManager::add(const std::string& insertPath,
							const std::string& name,
							eMenuItemType type,
							const std::string& caption,
							const std::string& icon,
							const std::string& eventName)
{
	// Sanity check for empty menu
	if (_root == NULL) return NULL;

	MenuItemPtr found = _root->find(insertPath);

	if (found != NULL)
	{
		// Allocate a new MenuItem
		MenuItemPtr newItem = MenuItemPtr(new MenuItem(found));

		newItem->setName(name);
		newItem->setCaption(caption);
		newItem->setType(type);
		newItem->setIcon(icon);
		newItem->setEvent(eventName);

		// Get the parent widget
		Gtk::Widget* parentItem = found->getWidget();
		Gtk::MenuShell* parent = NULL;

		if (type == menuFolder)
		{
			parent = static_cast<Gtk::MenuShell*>(parentItem);
		}
		else
		{
			// Retrieve the submenu widget from the item
			Gtk::MenuItem* menuItem = dynamic_cast<Gtk::MenuItem*>(parentItem);

			if (menuItem != NULL)
			{
				parent = menuItem->get_submenu();
			}
			else
			{
				rError() << "Cannot cast parent item to a Gtk::MenuItem*." << std::endl;
			}
		}

		Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(newItem->getWidget());

		if (item != NULL)
		{
			parent->append(*item);
		}
		else
		{
			rError() << "Cannot cast item to a Gtk::MenuItem*." << std::endl;
		}

		// Add the child to the <found> parent, AFTER its GtkWidget* operator
		// was invoked, otherwise the parent tries to instantiate it before it's actually
		// added.
		found->addChild(newItem);

		return newItem->getWidget();
	}
	else if (insertPath.empty())
	{
		// We have a new top-level menu item, create it as child of root
		MenuItemPtr newItem = MenuItemPtr(new MenuItem(_root));

		newItem->setName(name);
		newItem->setCaption(caption);
		newItem->setType(type);
		newItem->setIcon(icon);
		newItem->setEvent(eventName);

		// Insert into root
		_root->addChild(newItem);

		return newItem->getWidget();
	}
	else {
		// not found and not a top-level item either.
	}

	return NULL;
}

Gtk::Widget* MenuManager::insert(const std::string& insertPath,
						 const std::string& name,
						 eMenuItemType type,
						 const std::string& caption,
						 const std::string& icon,
						 const std::string& eventName)
{
	// Sanity check for empty menu
	if (_root == NULL) return NULL;

	MenuItemPtr found = _root->find(insertPath);

	if (found != NULL)
	{
		if (found->parent() != NULL)
		{
			// Get the GTK Menu position of the child widget
			int position = found->parent()->getMenuPosition(found);
			// Allocate a new MenuItem
			MenuItemPtr newItem = MenuItemPtr(new MenuItem(found->parent()));
			found->parent()->addChild(newItem);

			// Load the properties into the new child
			newItem->setName(name);
			newItem->setType(type);
			newItem->setCaption(caption);
			newItem->setEvent(eventName);
			newItem->setIcon(icon);

			Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(newItem->getWidget());

			if (item == NULL)
			{
				rError() << "Cannot cast item to a Gtk::MenuItem*." << std::endl;
				return NULL;
			}

			Gtk::Widget* parentWidget = found->parent()->getWidget();

			// Insert it at the given position
			if (found->parent()->getType() == menuBar)
			{
				// The parent is a menubar, it's a menushell in the first place
				static_cast<Gtk::MenuShell*>(parentWidget)->insert(*item, position);
			}
			else if (found->parent()->getType() == menuFolder)
			{
				// The parent is a submenu (=menuitem), try to retrieve the menushell first
				Gtk::MenuItem* menuItem = dynamic_cast<Gtk::MenuItem*>(parentWidget);

				if (menuItem != NULL)
				{
					menuItem->get_submenu()->insert(*item, position);
				}
				else
				{
					rError() << "Cannot cast parent item to a Gtk::MenuItem*." << std::endl;
				}
			}

			return newItem->getWidget();
		}
		else {
			rError() << "MenuManager: Unparented menuitem, can't determine position: ";
			rError() << insertPath << std::endl;
			return NULL;
		}
	}
	else {
		rError() << "MenuManager: Could not find insertPath: " << insertPath << std::endl;
		return NULL;
	}
}

void MenuManager::remove(const std::string& path)
{
	// Sanity check for empty menu
	if (_root == NULL) return;

	MenuItemPtr item = _root->find(path);

	if (item == NULL) return; // nothing to do

	MenuItemPtr parent = item->parent();

	if (parent == NULL) return; // no parent ?

	// Get the parent Gtk::Widget*
	Gtk::Widget* parentWidget = parent->getWidget();

	// Remove the found item from the parent menu item
	parent->removeChild(item);

	Gtk::MenuShell* shell = NULL;

	if (parent->getType() == menuBar)
	{
		// The parent is a menubar, it's a menushell in the first place
		shell = dynamic_cast<Gtk::MenuShell*>(parentWidget);
	}
	else if (parent->getType() == menuFolder)
	{
		// The parent is a submenu (=menuitem), try to retrieve the menushell first
		shell = dynamic_cast<Gtk::MenuShell*>(static_cast<Gtk::MenuItem*>(parentWidget)->get_submenu());
	}

	if (shell != NULL)
	{
		// Cast the item onto a GtkWidget to remove it from the parent container
		shell->remove(*item->getWidget());
	}
}

void MenuManager::updateAccelerators()
{
	// Sanity check for empty menu
	if (_root == NULL) return;

	_root->updateAcceleratorRecursive();
}

} // namespace ui
