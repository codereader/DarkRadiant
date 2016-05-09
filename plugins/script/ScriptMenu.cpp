#include "ScriptMenu.h"

#include "iuimanager.h"
#include "i18n.h"
#include <map>

namespace ui
{

const char* const SCRIPT_MENU_CAPTION = "&Scripts";
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
        // Let's sort the commands by display name
        typedef std::multimap<std::string, script::ScriptCommandPtr> SortedCommands;
        SortedCommands sortedCommands;

        for (script::ScriptCommandMap::value_type pair : commands)
        {
            if (pair.first == "Example") continue; // skip the example script

            sortedCommands.insert(script::ScriptCommandMap::value_type(pair.second->getDisplayName(), pair.second));
        }

		for (SortedCommands::value_type pair : sortedCommands)
		{
			menuManager.add(
				SCRIPT_MENU_PATH,
				"script" + pair.second->getName(),
				menuItem,
				pair.second->getDisplayName(),
				"",
				pair.second->getName()
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
