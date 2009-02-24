#ifndef _LAYOUT_COMMAND_H_
#define _LAYOUT_COMMAND_H_

#include "ieventmanager.h"
#include "iuimanager.h"
#include "icommandsystem.h"
#include "imainframe.h"

#include "generic/callback.h"

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
 * a callback for toggling a certain layout. The class
 * registers itself with the EventManager on construction.
 */
class LayoutCommand
{
	std::string _layoutName;
public:
	LayoutCommand(const std::string& layoutName) :
		_layoutName(layoutName) 
	{
		GlobalCommandSystem().addCommand(
			std::string("ToggleLayout") + _layoutName,
			boost::bind(&LayoutCommand::toggleLayout, this, _1)
		);
		GlobalEventManager().addCommand(
			std::string("ToggleLayout") + _layoutName,
			std::string("ToggleLayout") + _layoutName
		);

		std::string activateEventName = "ActivateLayout" + _layoutName;
		GlobalCommandSystem().addCommand(
			activateEventName,
			boost::bind(&LayoutCommand::activateLayout, this, _1)
		);
		GlobalEventManager().addCommand(activateEventName, activateEventName);

		// Add commands to menu
		IMenuManager& menuManager = GlobalUIManager().getMenuManager();

		// Add a new folder, if not existing yet
		if (menuManager.get(MENU_LAYOUTS_PATH) == NULL) {
			menuManager.insert(
				MENU_LAYOUTS_INSERT_BEFORE, 
				MENU_LAYOUTS, 
				menuFolder, "Window Layout", 
				"", "" // no icon, no event
			);	
		}

		// Add the item
		menuManager.add(MENU_LAYOUTS_PATH, _layoutName, menuItem, _layoutName, "", activateEventName);
	}

	~LayoutCommand(){
		// Remove command from menu
		IMenuManager& menuManager = GlobalUIManager().getMenuManager();
		menuManager.remove(MENU_LAYOUTS_PATH + "/" + _layoutName);

		// Remove the events
		GlobalEventManager().removeEvent(std::string("ToggleLayout") + _layoutName);
		GlobalEventManager().removeEvent(std::string("ActivateLayout") + _layoutName);

		GlobalCommandSystem().removeCommand(std::string("ToggleLayout") + _layoutName);
		GlobalCommandSystem().removeCommand(std::string("ToggleLayout") + _layoutName);
	}

	// Command target for activating the layout
	void activateLayout(const cmd::ArgumentList& args) {
		GlobalMainFrame().applyLayout(_layoutName);
	}

	// Command target for toggling the layout
	void toggleLayout(const cmd::ArgumentList& args) {
		// Check if active
		if (GlobalMainFrame().getCurrentLayout() == _layoutName) {
			// Remove the active layout
			GlobalMainFrame().applyLayout("");
		}
		else {
			GlobalMainFrame().applyLayout(_layoutName);
		}
	}
};
typedef boost::shared_ptr<LayoutCommand> LayoutCommandPtr;

} // namespace ui

#endif /* _LAYOUT_COMMAND_H_ */
