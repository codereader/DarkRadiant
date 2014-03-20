#include "Statement.h"

#include "icommandsystem.h"
#include <gtkmm/menuitem.h>
#include <gtkmm/button.h>
#include <gtkmm/toolbutton.h>

#include "itextstream.h"
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/toolbar.h>
#include <wx/button.h>

Statement::Statement(const std::string& statement, bool reactOnKeyUp) :
	_statement(statement),
	_reactOnKeyUp(reactOnKeyUp)
{}

Statement::~Statement()
{
	for (WidgetList::iterator i = _connectedWidgets.begin(); i != _connectedWidgets.end(); ++i)
	{
		i->second.disconnect();
	}
}

bool Statement::empty() const
{
	return false;
}

// Invoke the registered callback
void Statement::execute()
{
	if (_enabled) {
		GlobalCommandSystem().execute(_statement);
	}
}

// Override the derived keyUp method
void Statement::keyUp()
{
	if (_reactOnKeyUp) {
		// Execute the Statement on key up event
		execute();
	}
}

// Override the derived keyDown method
void Statement::keyDown()
{
	if (!_reactOnKeyUp) {
		// Execute the Statement on key down event
		execute();
	}
}

// Connect the given menuitem or toolbutton to this Statement
void Statement::connectWidget(Gtk::Widget* widget)
{
	if (dynamic_cast<Gtk::MenuItem*>(widget) != NULL)
	{
		// Connect the callback function
		Gtk::MenuItem* menuItem = static_cast<Gtk::MenuItem*>(widget);

		_connectedWidgets[widget] = menuItem->signal_activate().connect(
			sigc::mem_fun(*this, &Statement::onMenuItemClicked));
	}
	else if (dynamic_cast<Gtk::ToolButton*>(widget) != NULL)
	{
		// Connect the callback function
		Gtk::ToolButton* toolButton = static_cast<Gtk::ToolButton*>(widget);

		_connectedWidgets[widget] = toolButton->signal_clicked().connect(
			sigc::mem_fun(*this, &Statement::onToolButtonPress));
	}
	else if (dynamic_cast<Gtk::Button*>(widget) != NULL)
	{
		Gtk::Button* button = static_cast<Gtk::Button*>(widget);

		_connectedWidgets[widget] = button->signal_clicked().connect(
			sigc::mem_fun(*this, &Statement::onButtonPress));
	}
}

void Statement::connectMenuItem(wxMenuItem* item)
{
	if (_menuItems.find(item) != _menuItems.end())
	{
		rWarning() << "Cannot connect to the same menu item more than once." << std::endl;
		return;
	}

	_menuItems.insert(item);

	// Connect the event to the callback of this class
	assert(item->GetMenu());
	item->GetMenu()->Connect(wxEVT_MENU, wxCommandEventHandler(Statement::onWxMenuItemClicked), NULL, this);
}

void Statement::disconnectMenuItem(wxMenuItem* item)
{
	if (_menuItems.find(item) == _menuItems.end())
	{
		rWarning() << "Cannot disconnect from unconnected menu item." << std::endl;
		return;
	}

	_menuItems.erase(item);

	// Connect the event to the callback of this class
	assert(item->GetMenu());
	item->GetMenu()->Disconnect(wxEVT_MENU, wxCommandEventHandler(Statement::onWxMenuItemClicked), NULL, this);
}

void Statement::onWxMenuItemClicked(wxCommandEvent& ev)
{
	// Make sure the event is actually directed at us
	for (MenuItems::const_iterator i = _menuItems.begin(); i != _menuItems.end(); ++i)
	{
		if ((*i)->GetId() == ev.GetId())
		{
			execute();
			return;
		}
	}

	ev.Skip();
}

void Statement::connectToolItem(wxToolBarToolBase* item)
{
	if (_toolItems.find(item) != _toolItems.end())
	{
		rWarning() << "Cannot connect to the same tool item more than once." << std::endl;
		return;
	}

	_toolItems.insert(item);

	// Connect the to the callback of this class
	item->GetToolBar()->Connect(wxEVT_TOOL, wxCommandEventHandler(Statement::onWxToolItemClicked), NULL, this);
}

void Statement::disconnectToolItem(wxToolBarToolBase* item)
{
	if (_toolItems.find(item) == _toolItems.end())
	{
		rWarning() << "Cannot disconnect from unconnected tool item." << std::endl;
		return;
	}

	_toolItems.erase(item);

	// Connect the to the callback of this class
	item->GetToolBar()->Disconnect(wxEVT_TOOL, wxCommandEventHandler(Statement::onWxToolItemClicked), NULL, this);
}

void Statement::onWxToolItemClicked(wxCommandEvent& ev)
{
	// Make sure the event is actually directed at us
	for (ToolItems::const_iterator i = _toolItems.begin(); i != _toolItems.end(); ++i)
	{
		if ((*i)->GetId() == ev.GetId())
		{
			execute();
			return;
		}
	}

	ev.Skip();
}

void Statement::connectButton(wxButton* button)
{
	if (_buttons.find(button) != _buttons.end())
	{
		rWarning() << "Cannot connect to the same button more than once." << std::endl;
		return;
	}

	_buttons.insert(button);

	// Connect the to the callback of this class
	button->Connect(wxEVT_BUTTON, wxCommandEventHandler(Statement::onWxButtonClicked), NULL, this);
}

void Statement::disconnectButton(wxButton* button)
{
	if (_buttons.find(button) == _buttons.end())
	{
		rWarning() << "Cannot disconnect from unconnected button." << std::endl;
		return;
	}

	_buttons.erase(button);

	// Connect the to the callback of this class
	button->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(Statement::onWxButtonClicked), NULL, this);
}

void Statement::onWxButtonClicked(wxCommandEvent& ev)
{
	// Execute the Statement
	execute();
}

void Statement::onButtonPress()
{
	// Execute the Statement
	execute();
}

void Statement::onToolButtonPress()
{
	// Execute the Statement
	execute();
}

void Statement::onMenuItemClicked()
{
	// Execute the Statement
	execute();
}
