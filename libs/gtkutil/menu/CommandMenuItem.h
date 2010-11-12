#ifndef _GTKUTIL_COMMAND_MENU_ITEM_H_
#define _GTKUTIL_COMMAND_MENU_ITEM_H_

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

	void executeCommand()
	{
		GlobalCommandSystem().execute(_statementName);
	}
};
typedef boost::shared_ptr<CommandMenuItem> CommandMenuItemPtr;

} // namespace

#endif /* _GTKUTIL_COMMAND_MENU_ITEM_H_ */
