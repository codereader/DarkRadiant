#include "ScriptMenu.h"

#include "imenumanager.h"
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
	menu::IMenuManager& menuManager = GlobalMenuManager();

	// Create a new "folder"
	menuManager.insert(
		SCRIPT_MENU_INSERT_POINT,
		SCRIPT_MENU_NAME,
		menu::ItemType::Folder,
		_(SCRIPT_MENU_CAPTION),
		"",
		""
	);

	// Let's sort the commands by display name, map display name => name
	typedef std::multimap<std::string, std::string> SortedCommands;
	SortedCommands sortedCommands;

	GlobalScriptingSystem().foreachScriptCommand([&](const script::IScriptCommand& cmd)
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
				menu::ItemType::Item,
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
            menu::ItemType::Item,
			_("No scripts available"),
			"",
			""
		);
	}
}

ScriptMenu::~ScriptMenu()
{
	GlobalMenuManager().remove(SCRIPT_MENU_PATH);
}

} // namespace ui
