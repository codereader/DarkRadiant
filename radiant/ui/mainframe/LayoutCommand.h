#pragma once

#include "i18n.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "icommandsystem.h"
#include "imainframe.h"

#include "gtkutil/dialog/MessageBox.h"

#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

namespace ui {

	namespace {
		const std::string MENU_LAYOUTS_PARENT = "main/view";
		const std::string MENU_LAYOUTS = "layouts";
		const std::string MENU_LAYOUTS_PATH = MENU_LAYOUTS_PARENT + "/" + MENU_LAYOUTS;
		const std::string MENU_LAYOUTS_INSERT_BEFORE = "main/view/camera";
	}

/**
 * This little class represents a "command target", providing
 * a callback for activating a certain layout. The class
 * registers itself with the EventManager on construction.
 */
class LayoutCommand
{
	std::string _layoutName;

	std::string _activateCommand;
public:
	LayoutCommand(const std::string& layoutName) :
		_layoutName(layoutName)
	{
		_activateCommand = "ActivateLayout" + _layoutName;
		GlobalCommandSystem().addCommand(
			_activateCommand,
			boost::bind(&LayoutCommand::activateLayout, this, _1)
		);
		GlobalEventManager().addCommand(_activateCommand, _activateCommand);

		// Add commands to menu
		IMenuManager& menuManager = GlobalUIManager().getMenuManager();

		// Add a new folder, if not existing yet
		if (menuManager.get(MENU_LAYOUTS_PATH) == NULL) {
			menuManager.insert(
				MENU_LAYOUTS_INSERT_BEFORE,
				MENU_LAYOUTS,
				menuFolder, _("Window Layout"),
				"", "" // no icon, no event
			);
		}

		// Add the item
		menuManager.add(MENU_LAYOUTS_PATH, _layoutName, menuItem, _layoutName, "", _activateCommand);
	}

	~LayoutCommand()
	{
		// Remove command from menu
		IMenuManager& menuManager = GlobalUIManager().getMenuManager();
		menuManager.remove(MENU_LAYOUTS_PATH + "/" + _layoutName);

		// Remove event and command
		GlobalEventManager().removeEvent(_activateCommand);
		GlobalCommandSystem().removeCommand(_activateCommand);
	}

	// Command target for activating the layout
	void activateLayout(const cmd::ArgumentList& args)
	{
		GlobalMainFrame().setActiveLayoutName(_layoutName);
        gtkutil::MessageBox infoBox("Restart required",
                                    "Restart DarkRadiant to apply changes",
                                    ui::IDialog::MESSAGE_CONFIRM);
        infoBox.run();
	}
};
typedef boost::shared_ptr<LayoutCommand> LayoutCommandPtr;

} // namespace ui
