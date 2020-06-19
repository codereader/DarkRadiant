#pragma once

#include "imru.h"
#include "iuimanager.h"

#include <list>
#include <sigc++/connection.h>
#include <sigc++/functors/mem_fun.h>
#include <fmt/format.h>
#include "string/convert.h"

namespace ui
{

namespace
{
	const char* const RECENT_FILES_CAPTION = N_("Recently used Maps");
}

class MRUMenu
{
private:
	sigc::connection _mruUpdateConn;

public:
	MRUMenu()
	{
		_mruUpdateConn = GlobalMRU().signal_MapListChanged().connect(
			sigc::mem_fun(*this, &MRUMenu::onMRUListChanged)
		);

		constructMenu();
	}

	~MRUMenu()
	{
		clearMenu();

		_mruUpdateConn.disconnect();
	}

private:
	void clearMenu()
	{
		IMenuManager& menuManager = GlobalUIManager().getMenuManager();

		// Remove all items from the menu
		for (std::size_t i = GlobalMRU().getMaxNumberOfItems(); i > 0; i--)
		{
			menuManager.remove("main/file/MRU" + string::to_string(i));
		}

		menuManager.remove("main/file/mruempty");
	}

	void updateMenu()
	{
		clearMenu();

		std::vector<std::string> filenames;
		filenames.reserve(GlobalMRU().getMaxNumberOfItems());

		GlobalMRU().foreachItem([&](std::size_t index, const std::string& filename)
		{
			filenames.emplace_back(filename);
		});

		// Get the menumanager
		IMenuManager& menuManager = GlobalUIManager().getMenuManager();

		if (filenames.empty())
		{
			// Create the "empty" MRU menu item (the desensitised one)
			menuManager.insert(
				"main/file/mruseparator",
				"mruempty",
				ui::menuItem,
				RECENT_FILES_CAPTION,
				"", // empty icon
				"" // empty event
			);

			return;
		}

		// Add one item for each filename in the MRU list
		for (std::size_t index = 0; index < filenames.size(); ++index)
		{
			// The default string to be loaded into the widget (i.e. "inactive")
			std::string filename = filenames[index];

			std::string label = string::to_string(index + 1) + " - " + filename;

			auto statement = fmt::format(map::LOAD_MRU_STATEMENT_FORMAT, index + 1);

			// Create the menu item
			menuManager.insert(
				"main/file/mruseparator",
				"MRU" + string::to_string(index + 1),
				ui::menuItem,
				label,
				"", // empty icon
				statement
			);
		}
	}

	void constructMenu()
	{
		// Insert the last separator to split the MRU file list from the "Exit" command.
		GlobalUIManager().getMenuManager().insert(
			"main/file/exit",
			"mruseparator",
			ui::menuSeparator,
			"", // empty caption
			"", // empty icon
			"" // empty command
		);

		// Call the update routine to load the values into the widgets
		updateMenu();
	}

	void onMRUListChanged()
	{
		updateMenu();
	}
};

}
