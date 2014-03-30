#pragma once

#include "MenuItem.h"
#include "icommandsystem.h"

#include <boost/bind.hpp>

namespace gtkutil
{

/**
 * A specialised MenuItem invoking a specific command when clicked.
 */
class CommandMenuItem :
	public MenuItem
{
protected:
	std::string _statementName;

public:
	CommandMenuItem(Gtk::MenuItem* menuItem,
					const std::string& statementName,
					const ui::IMenu::SensitivityTest& sensTest = AlwaysSensitive,
					const ui::IMenu::VisibilityTest& visTest = AlwaysVisible)
	: MenuItem(menuItem,
			   boost::bind(&CommandMenuItem::executeCommand, this),
			   sensTest,
			   visTest),
	  _statementName(statementName)
	{}

	CommandMenuItem(wxMenuItem* menuItem,
					const std::string& statementName,
					const ui::IMenu::SensitivityTest& sensTest = AlwaysSensitive,
					const ui::IMenu::VisibilityTest& visTest = AlwaysVisible)
	: MenuItem(menuItem,
			   boost::bind(&CommandMenuItem::executeCommand, this),
			   sensTest,
			   visTest),
	  _statementName(statementName)
	{}

	void executeCommand()
	{
		GlobalCommandSystem().execute(_statementName);
	}
};
typedef std::shared_ptr<CommandMenuItem> CommandMenuItemPtr;

} // namespace
