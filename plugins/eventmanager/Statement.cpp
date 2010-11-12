#include "Statement.h"

#include "icommandsystem.h"
#include <gtkmm/menuitem.h>
#include <gtkmm/button.h>
#include <gtkmm/toolbutton.h>

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
