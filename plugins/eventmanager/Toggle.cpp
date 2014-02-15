#include "Toggle.h"

#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/togglebutton.h>

#include "itextstream.h"
#include <wx/menu.h>
#include <wx/menuitem.h>

Toggle::Toggle(const ToggleCallback& callback) :
	_callback(callback),
	_callbackActive(false),
	_toggled(false)
{}

Toggle::~Toggle()
{
	for (ToggleWidgetList::iterator i = _toggleWidgets.begin(); i != _toggleWidgets.end(); ++i)
	{
		i->second.disconnect();
	}
}

bool Toggle::empty() const {
	return false;
}

bool Toggle::isToggle() const {
	return true;
}

// Set the toggled state to true/false, according to <toggled> and update
// any associated widgets or notify any callbacks.
bool Toggle::setToggled(const bool toggled)
{
	if (_callbackActive) {
		return false;
	}

	// Update the toggle status and export it to the GTK button
	_toggled = toggled;
	updateWidgets();

	return true;
}

void Toggle::updateWidgets()
{
	_callbackActive = true;

	for (ToggleWidgetList::iterator i = _toggleWidgets.begin();
		 i != _toggleWidgets.end();
		 ++i)
	{
		Gtk::Widget* widget = i->first;

		if (dynamic_cast<Gtk::ToggleToolButton*>(widget) != NULL)
		{
			static_cast<Gtk::ToggleToolButton*>(widget)->set_active(_toggled);
		}
		else if (dynamic_cast<Gtk::ToggleButton*>(widget) != NULL)
		{
			static_cast<Gtk::ToggleButton*>(widget)->set_active(_toggled);
		}
		else if (dynamic_cast<Gtk::CheckMenuItem*>(widget) != NULL)
		{
			static_cast<Gtk::CheckMenuItem*>(widget)->set_active(_toggled);
		}
	}

	_callbackActive = false;
}

// On key press >> toggle the internal state
void Toggle::keyDown() {
	toggle();
}

bool Toggle::isToggled() const {
	return _toggled;
}

void Toggle::connectWidget(Gtk::Widget* widget)
{
	if (dynamic_cast<Gtk::ToggleToolButton*>(widget) != NULL)
	{
		Gtk::ToggleToolButton* toolButton = static_cast<Gtk::ToggleToolButton*>(widget);

		toolButton->set_active(_toggled);

		// Connect the toggleToolbutton to the callback of this class
		_toggleWidgets[widget] = toolButton->signal_toggled().connect(
			sigc::mem_fun(*this, &Toggle::onToggleToolButtonClicked));
	}
	else if (dynamic_cast<Gtk::ToggleButton*>(widget) != NULL)
	{
		Gtk::ToggleButton* toggleButton = static_cast<Gtk::ToggleButton*>(widget);

		toggleButton->set_active(_toggled);

		// Connect the togglebutton to the callback of this class
		_toggleWidgets[widget] = toggleButton->signal_toggled().connect(
			sigc::mem_fun(*this, &Toggle::onToggleButtonClicked));
	}
	else if (dynamic_cast<Gtk::CheckMenuItem*>(widget) != NULL)
	{
		Gtk::CheckMenuItem* menuItem = static_cast<Gtk::CheckMenuItem*>(widget);

		menuItem->set_active(_toggled);

		// Connect the togglebutton to the callback of this class
		_toggleWidgets[widget] = menuItem->signal_toggled().connect(
			sigc::mem_fun(*this, &Toggle::onCheckMenuItemClicked));
	}
}

void Toggle::disconnectWidget(Gtk::Widget* widget)
{
	ToggleWidgetList::iterator i = _toggleWidgets.find(widget);

	if (i != _toggleWidgets.end())
	{
		// Disconnect the signal
		i->second.disconnect();

		// Erase from the list
		_toggleWidgets.erase(i);
	}
}

void Toggle::connectMenuItem(wxMenuItem* item)
{
	if (!item->IsCheckable())
	{
		rWarning() << "Cannot connect non-checkable menu item to this event." << std::endl;
		return;
	}

	if (_menuItems.find(item) != _menuItems.end())
	{
		rWarning() << "Cannot connect to the same menu item more than once." << std::endl;
		return;
	}

	_menuItems.insert(item);

	item->Check(_toggled);

	// Connect the togglebutton to the callback of this class
	assert(item->GetMenu());
	item->GetMenu()->Connect(wxEVT_MENU, wxCommandEventHandler(Toggle::onMenuItemClicked), NULL, this);
}

void Toggle::disconnectMenuItem(wxMenuItem* item)
{
	if (!item->IsCheckable())
	{
		rWarning() << "Cannot disconnect from non-checkable menu item." << std::endl;
		return;
	}

	if (_menuItems.find(item) == _menuItems.end())
	{
		rWarning() << "Cannot disconnect from unconnected menu item." << std::endl;
		return;
	}

	_menuItems.erase(item);

	// Connect the togglebutton to the callback of this class
	assert(item->GetMenu());
	item->GetMenu()->Disconnect(wxEVT_MENU, wxCommandEventHandler(Toggle::onMenuItemClicked), NULL, this);
}

void Toggle::onMenuItemClicked(wxCommandEvent& ev)
{
	// Make sure the event is actually directed at us
	for (MenuItems::const_iterator i = _menuItems.begin(); i != _menuItems.end(); ++i)
	{
		if ((*i)->GetId() == ev.GetId())
		{
			toggle();
			return;
		}
	}

	ev.Skip();
}

// Invoke the registered callback and update/notify
void Toggle::toggle()
{
	if (_callbackActive) {
		return;
	}

	// Check if the toggle event is enabled
	if (_enabled) {
		// Invert the <toggled> state
		_toggled = !_toggled;

		// Call the connected function with the new state
		_callback(_toggled);
	}

	// Update any attached GtkObjects in any case
	updateWidgets();
}

void Toggle::onToggleToolButtonClicked()
{
	toggle();
}

void Toggle::onToggleButtonClicked()
{
	toggle();
}

void Toggle::onCheckMenuItemClicked()
{
	toggle();
}
