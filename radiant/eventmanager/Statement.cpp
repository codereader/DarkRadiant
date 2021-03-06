#include "Statement.h"

#include "icommandsystem.h"

#include "itextstream.h"
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/toolbar.h>
#include <wx/button.h>
#include "Accelerator.h"

namespace ui
{

Statement::Statement(const std::string& statement, bool reactOnKeyUp) :
	_statement(statement),
	_reactOnKeyUp(reactOnKeyUp)
{}

Statement::~Statement()
{
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
	item->GetMenu()->Connect(item->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(Statement::onMenuItemClicked), NULL, this);
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
	item->GetMenu()->Disconnect(item->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(Statement::onMenuItemClicked), NULL, this);
}

void Statement::onMenuItemClicked(wxCommandEvent& ev)
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

void Statement::connectToolItem(const wxToolBarToolBase* item)
{
	if (_toolItems.find(item) != _toolItems.end())
	{
		rWarning() << "Cannot connect to the same tool item more than once." << std::endl;
		return;
	}

	_toolItems.insert(item);

	// Connect the to the callback of this class
	item->GetToolBar()->Bind(wxEVT_TOOL, &Statement::onToolItemClicked, this, item->GetId());
}

void Statement::disconnectToolItem(const wxToolBarToolBase* item)
{
	if (_toolItems.find(item) == _toolItems.end())
	{
		//rWarning() << "Cannot disconnect from unconnected tool item." << std::endl;
		return;
	}

	_toolItems.erase(item);

	// Connect the to the callback of this class
	item->GetToolBar()->Unbind(wxEVT_TOOL, &Statement::onToolItemClicked, this, item->GetId());
}

void Statement::onToolItemClicked(wxCommandEvent& ev)
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

}
