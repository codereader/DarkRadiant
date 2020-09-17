#include "ScriptMenu.h"

#include "iuimanager.h"
#include "iscript.h"
#include "i18n.h"
#include <map>

namespace ui
{

const char* const SCRIPT_MENU_CAPTION = "&Scripts";
const std::string SCRIPT_MENU_NAME = "scripts";
const std::string SCRIPT_MENU_INSERT_POINT = "main/help";
const std::string SCRIPT_MENU_PATH = "main/scripts";

ScriptMenu::ScriptMenu()
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

	// Let's sort the commands by display name, map display name => name
	typedef std::multimap<std::string, std::string> SortedCommands;
	SortedCommands sortedCommands;

	GlobalScriptingSystem().foreachScriptCommand([&](const IScriptCommand& cmd)
	{
		sortedCommands.insert(std::make_pair(cmd.getDisplayName(), cmd.getName()));
	});

	if (!sortedCommands.empty())
	{
		for (const auto& pair : sortedCommands)
		{
			menuManager.add(
				SCRIPT_MENU_PATH,
				"script" + pair.second,
				menuItem,
				pair.first,
				"",
				pair.second
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
