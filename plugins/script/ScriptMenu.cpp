#include "ScriptMenu.h"

#include "iuimanager.h"
#include "i18n.h"

namespace ui
{

const char* const SCRIPT_MENU_CAPTION = "_Scripts";
const std::string SCRIPT_MENU_NAME = "scripts";
const std::string SCRIPT_MENU_INSERT_POINT = "main/help";
const std::string SCRIPT_MENU_PATH = "main/scripts";

ScriptMenu::ScriptMenu(const script::ScriptCommandMap& commands)
{
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	// Create a new "folder"
	menuManager.insert(
		SCRIPT_MENU_INSERT_POINT,
		SCRIPT_MENU_NAME,
		menuFolder,
		_(SCRIPT_MENU_CAPTION),
		"",
		""
	);

	if (!commands.empty())
	{
		for (script::ScriptCommandMap::const_iterator i = commands.begin();
			 i != commands.end(); ++i)
		{
			if (i->first == "Example") continue; // skip the example script

			menuManager.add(
				SCRIPT_MENU_PATH,
				"script" + i->first,
				menuItem,
				i->second->getDisplayName(),
				"",
				i->first
			);
		}
	}
	else
	{
		menuManager.add(
			SCRIPT_MENU_PATH,
			"noscriptsavailable",
			menuItem,
			_("No scripts available"),
			"",
			""
		);
	}
}

ScriptMenu::~ScriptMenu()
{
	GlobalUIManager().getMenuManager().remove(SCRIPT_MENU_PATH);
}

} // namespace ui
