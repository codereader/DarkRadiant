#include "MenuItem.h"

#include "i18n.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <gtkmm/menushell.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/menubar.h>
#include <gtkmm/menu.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/widget.h>

#include <iostream>

namespace ui
{

	namespace {
		typedef std::vector<std::string> StringVector;
	}

MenuItem::MenuItem(const MenuItemPtr& parent) :
	_parent(MenuItemWeakPtr(parent)),
	_menuItem(NULL),
	_widget(NULL),
	_type(menuNothing),
	_constructed(false)
{
	if (parent == NULL) {
		_type = menuRoot;
	}
	else if (parent->isRoot()) {
		_type = menuBar;
	}
}

MenuItem::~MenuItem() {
	if (!_event.empty()) {
		IEventPtr ev = GlobalEventManager().findEvent(_event);

		// Tell the eventmanager to disconnect the widget in any case
		// even if has been destroyed already.
		if (ev != NULL) {
			ev->disconnectWidget(_widget);
		}
	}
}

std::string MenuItem::getName() const {
	return _name;
}

void MenuItem::setName(const std::string& name) {
	_name = name;
}

bool MenuItem::isRoot() const {
	return (_type == menuRoot);
}

MenuItemPtr MenuItem::parent() const {
	return _parent.lock();
}

void MenuItem::setParent(const MenuItemPtr& parent) {
	_parent = parent;
}

void MenuItem::setCaption(const std::string& caption) {
	_caption = caption;
}

std::string MenuItem::getCaption() const {
	return _caption;
}

void MenuItem::setIcon(const std::string& icon) {
	_icon = icon;
}

bool MenuItem::isEmpty() const {
	return (_type != menuItem);
}

eMenuItemType MenuItem::getType() const {
	return _type;
}

void MenuItem::setType(eMenuItemType type) {
	_type = type;
}

std::size_t MenuItem::numChildren() const {
	return _children.size();
}

void MenuItem::addChild(const MenuItemPtr& newChild) {
	_children.push_back(newChild);
}

void MenuItem::removeChild(const MenuItemPtr& child)
{
	for (MenuItemList::iterator i = _children.begin(); i != _children.end(); ++i)
	{
		if (*i == child)
		{
			_children.erase(i);
			return;
		}
	}
}

std::string MenuItem::getEvent() const
{
	return _event;
}

void MenuItem::setEvent(const std::string& eventName)
{
	_event = eventName;
}

int MenuItem::getMenuPosition(const MenuItemPtr& child)
{
	if (!_constructed)
	{
		construct();
	}

	// Check if this is the right item type for this operation
	if (_type == menuFolder || _type == menuBar)
	{
		Gtk::Container* container = dynamic_cast<Gtk::Container*>(_widget);

		// A menufolder is a menuitem with a contained submenu, retrieve it
		if (_type == menuFolder)
		{
			container = static_cast<Gtk::MenuItem*>(_widget)->get_submenu();
		}

		// Get the list of child widgets
		std::vector<Gtk::Widget*> children = container->get_children();

		// The child Widget for comparison
		Gtk::Widget* childWidget = child->getWidget();

		for (std::size_t i = 0; i < children.size(); ++i)
		{
			// Get the widget pointer from the current list item
			if (children[i] == childWidget)
			{
				return static_cast<int>(i);
			}
		}
	}

	return -1; // not found or wrong type
}

Gtk::Widget* MenuItem::getWidget()
{
	// Check for toggle, allocate the Gtk::Widget*
	if (!_constructed)
	{
		construct();
	}

	return _widget;
}

MenuItemPtr MenuItem::find(const std::string& menuPath)
{
	// Split the path and analyse it
	StringVector parts;
	boost::algorithm::split(parts, menuPath, boost::algorithm::is_any_of("/"));

	// Any path items at all?
	if (!parts.empty()) {
		MenuItemPtr child;

		// Path is not empty, try to find the first item among the item's children
		for (std::size_t i = 0; i < _children.size(); i++) {
			if (_children[i]->getName() == parts[0]) {
				child = _children[i];
				break;
			}
		}

		// The topmost name seems to be part of the children, pass the call
		if (child != NULL) {
			// Is this the end of the path (no more items)?
			if (parts.size() == 1) {
				// Yes, return the found item
				return child;
			}
			else {
				// No, pass the query down the hierarchy
				std::string childPath("");
				for (std::size_t i = 1; i < parts.size(); i++) {
					childPath += (childPath != "") ? "/" : "";
					childPath += parts[i];
				}
				return child->find(childPath);
			}
		}
	}

	// Nothing found, return NULL pointer
	return MenuItemPtr();
}

void MenuItem::parseNode(xml::Node& node, const MenuItemPtr& thisItem) {
	std::string nodeName = node.getName();

	setName(node.getAttributeValue("name"));

	// Put the caption through gettext before passing it to setCaption
	setCaption(_(node.getAttributeValue("caption").c_str()));

	if (nodeName == "menuItem") {
		_type = menuItem;
		// Get the EventPtr according to the event
		setEvent(node.getAttributeValue("command"));
		setIcon(node.getAttributeValue("icon"));
	}
	else if (nodeName == "menuSeparator") {
		_type = menuSeparator;
	}
	else if (nodeName == "subMenu") {
		_type = menuFolder;

		xml::NodeList subNodes = node.getChildren();
		for (std::size_t i = 0; i < subNodes.size(); i++) {
			if (subNodes[i].getName() != "text" && subNodes[i].getName() != "comment") {
				// Allocate a new child menuitem with a pointer to <self>
				MenuItemPtr newChild = MenuItemPtr(new MenuItem(thisItem));
				// Let the child parse the subnode
				newChild->parseNode(subNodes[i], newChild);

				// Add the child to the list
				_children.push_back(newChild);
			}
		}
	}
	else if (nodeName == "menu") {
		_type = menuBar;

		xml::NodeList subNodes = node.getChildren();
		for (std::size_t i = 0; i < subNodes.size(); i++) {
			if (subNodes[i].getName() != "text" && subNodes[i].getName() != "comment") {
				// Allocate a new child menuitem with a pointer to <self>
				MenuItemPtr newChild = MenuItemPtr(new MenuItem(thisItem));
				// Let the child parse the subnode
				newChild->parseNode(subNodes[i], newChild);

				// Add the child to the list
				_children.push_back(newChild);
			}
		}
	}
	else {
		_type = menuNothing;
		rError() << "MenuItem: Unknown node found: " << nodeName << std::endl;
	}
}

void MenuItem::construct()
{
	if (_type == menuBar)
	{
		Gtk::MenuBar* menuBar = Gtk::manage(new Gtk::MenuBar);
		_widget = menuBar;

		for (std::size_t i = 0; i < _children.size(); i++)
		{
			// Cast each children onto GtkWidget and append it to the menu
			Gtk::MenuItem* menuItem = dynamic_cast<Gtk::MenuItem*>(_children[i]->getWidget());

			if (menuItem != NULL)
			{
				menuBar->append(*menuItem);
			}
			else
			{
				rError() << "MenuItem::construct: Cannot cast child to Gtk::MenuItem" << std::endl;
			}
		}
	}
	else if (_type == menuSeparator)
	{
		_widget = Gtk::manage(new Gtk::SeparatorMenuItem);
	}
	else if (_type == menuFolder)
	{
		// Create the menuitem
		Gtk::MenuItem* menuItem = Gtk::manage(new Gtk::MenuItem(_caption, true));
		_widget = menuItem;

		// Create the submenu
		Gtk::Menu* subMenu = Gtk::manage(new Gtk::Menu);
		subMenu->show();

		// Attach the submenu to the menuitem
		menuItem->set_submenu(*subMenu);

		for (std::size_t i = 0; i < _children.size(); i++)
		{
			// Cast each children onto GtkWidget and append it to the menu
			Gtk::MenuItem* menuItem = dynamic_cast<Gtk::MenuItem*>(_children[i]->getWidget());

			if (menuItem != NULL)
			{
				subMenu->append(*menuItem);
			}
			else
			{
				rError() << "MenuItem::construct: Cannot cast child to Gtk::MenuItem" << std::endl;
			}
		}
	}
	else if (_type == menuItem)
	{
		if (!_event.empty())
		{
			// Try to lookup the event name
			IEventPtr event = GlobalEventManager().findEvent(_event);

			if (!event->empty()) {
				// Retrieve an accelerator string formatted for a menu
				const std::string accelText =
					GlobalEventManager().getAcceleratorStr(event, true);

				// Create a new menuitem
				if (event->isToggle())
				{
					gtkutil::TextToggleMenuItemAccelerator* menuItem = Gtk::manage(new gtkutil::TextToggleMenuItemAccelerator(
						_caption,
						accelText,
						!_icon.empty() ? GlobalUIManager().getLocalPixbuf(_icon) : Glib::RefPtr<Gdk::Pixbuf>()
					));

					_menuItem = menuItem;
					_widget = menuItem;
				}
				else
				{
					gtkutil::TextMenuItemAccelerator* menuItem = Gtk::manage(new gtkutil::TextMenuItemAccelerator(
						_caption,
						accelText,
						!_icon.empty() ? GlobalUIManager().getLocalPixbuf(_icon) : Glib::RefPtr<Gdk::Pixbuf>()
					));

					_menuItem = menuItem;
					_widget = menuItem;
				}

				_widget->show_all();

				// Connect the widget to the event
				event->connectWidget(_widget);
			}
			else
			{
				std::cout << "MenuItem: Cannot find associated event: " << _event << std::endl;
			}
		}
		else
		{
			// Create an empty, desensitised menuitem
			_widget = Gtk::manage(new gtkutil::TextMenuItemAccelerator(_caption, "", Glib::RefPtr<Gdk::Pixbuf>()));
			_widget->set_sensitive(false);
		}
	}
	else if (_type == menuRoot)
	{
		// Cannot instantiate root MenuItem, ignore
	}

	if (_widget != NULL)
	{
		_widget->show_all();
	}

	_constructed = true;
}

void MenuItem::updateAcceleratorRecursive()
{
	if (!_constructed) {
		construct();
	}

	if (_type == menuItem && _menuItem != NULL)
	{
		// Try to lookup the event name
		IEventPtr event = GlobalEventManager().findEvent(_event);

		if (!_event.empty() && event != NULL) {
			// Retrieve an accelerator string formatted for a menu
			const std::string accelText =
				GlobalEventManager().getAcceleratorStr(event, true);

			// Update the accelerator text on the existing menuitem
			_menuItem->setAccelerator(accelText);
		}
	}

	// Iterate over all the children and pass the call
	for (std::size_t i = 0; i < _children.size(); ++i)
	{
		_children[i]->updateAcceleratorRecursive();
	}
}

} // namespace ui
