#pragma once

#include "MenuItem.h"
#include "icommandsystem.h"

#include <functional>

namespace wxutil
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
	CommandMenuItem(wxMenuItem* menuItem,
					const std::string& statementName,
					const ui::IMenu::SensitivityTest& sensTest = AlwaysSensitive,
					const ui::IMenu::VisibilityTest& visTest = AlwaysVisible)
	: MenuItem(menuItem,
			   std::bind(&CommandMenuItem::executeCommand, this),
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
