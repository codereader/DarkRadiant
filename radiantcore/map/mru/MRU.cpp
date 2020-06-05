#include "MRU.h"

#include "i18n.h"
#include "icommandsystem.h"
#include "ipreferencesystem.h"

#include "string/string.h"
#include <fmt/format.h>
#include "os/file.h"
#include "registry/registry.h"
#include "command/ExecutionFailure.h"
#include "module/StaticModule.h"

namespace map
{

namespace
{
	const std::string RKEY_MAP_ROOT = "user/ui/map";
	const std::string RKEY_MAP_MRUS = RKEY_MAP_ROOT + "/MRU";
	const std::string RKEY_MRU_LENGTH = RKEY_MAP_ROOT + "/numMRU";

#if 0
	const char* const RECENT_FILES_CAPTION = N_("Recently used Maps");
#endif
}

#if 0
MRU::MRU() :
	_numMaxFiles(registry::getValue<int>(RKEY_MRU_LENGTH)),
	_list(_numMaxFiles)
#if 0
	,
	_emptyMenuItem(_(RECENT_FILES_CAPTION), *this, 0)
#endif
{
#if 0
	// Create _numMaxFiles menu items
	for (std::size_t i = 0; i < _numMaxFiles; i++) {

		_menuItems.push_back(MRUMenuItem(string::to_string(i), *this, i+1));

		MRUMenuItem& item = (*_menuItems.rbegin());

		const std::string commandName = std::string("MRUOpen") + string::to_string(i+1);

		// Connect the command to the last inserted menuItem
		GlobalCommandSystem().addCommand(
			commandName,
			std::bind(&MRUMenuItem::activate, &item, std::placeholders::_1)
		);
	}
#endif
}
#endif

sigc::signal<void>& MRU::signal_MapListChanged()
{
	return _signalMapListChanged;
}

void MRU::loadRecentFiles()
{
	// Loads the registry values from the last to the first (recentMap4 ... recentMap1) and
	// inserts them. After everything is loaded, the file list is sorted correctly.
	for (std::size_t i = _numMaxFiles; i > 0; i--)
	{
		const std::string key = RKEY_MAP_MRUS + "/map" + string::to_string(i);
		auto fileName = registry::getValue<std::string>(key);

		if (!fileName.empty())
		{
			// Insert the item into the filelist
			_list->insert(fileName);
		}
	}
}

void MRU::saveRecentFiles()
{
	// Delete all existing MRU/element nodes
	GlobalRegistry().deleteXPath(RKEY_MAP_MRUS);

	std::size_t counter = 1;

	// Now wade through the list and save them in the correct order
	for (MRUList::const_iterator i = _list->begin(); i != _list->end(); ++counter, ++i)
	{
		const std::string key = RKEY_MAP_MRUS + "/map" + string::to_string(counter);

		// Save the string into the registry
		registry::setValue(key, (*i));
	}
}

void MRU::loadMap(const std::string& fileName)
{
#if 0 // TODO
	if (GlobalMap().askForSave(_("Open Map")))
	{
		if (os::fileIsReadable(fileName))
		{
			// Shut down the current map
			GlobalMap().freeMap();

			// Load the file
			GlobalMap().load(fileName);

			// Update the MRU list with this file
			insert(fileName);
		}
		else
		{
			wxutil::Messagebox::ShowError(
				fmt::format(_("Could not read map file: {0}"), fileName));
		}
	}
#endif
}

// Construct the MRU preference page and add it to the given group
void MRU::constructPreferences()
{
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Map Files"));

	page.appendEntry(_("Number of most recently used files"), RKEY_MRU_LENGTH);
	page.appendCheckBox(_("Open last map on startup"), RKEY_LOAD_LAST_MAP);
}

#if 0
void MRU::initialise()
{
	// Construct the MRU commands and menu structure
	constructMenu();

	// Initialise the most recently used files list
	loadRecentFiles();

	updateMenu();
}
#endif

std::string MRU::getLastMapName()
{
	return _list->empty() ? "" : *_list->begin();
}

std::size_t MRU::getMaxNumberOfItems() const
{
	return _numMaxFiles;
}

void MRU::foreachItem(const ItemFunctor& functor)
{
	std::size_t counter = 1;

	// Now wade through the list and save them in the correct order
	for (auto i = _list->begin(); i != _list->end(); ++counter, ++i)
	{
		functor(counter, *i);
	}
}

void MRU::insert(const std::string& fileName) {

	if (!fileName.empty())
	{
		// Insert the item into the filelist
		_list->insert(fileName);

#if 0
		// Update the widgets
		updateMenu();
#endif
		// Persist MRU to the registry on each change
		saveRecentFiles();

		_signalMapListChanged.emit();
	}
}

#if 0
void MRU::updateMenu()
{
	// Get the menumanager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	for (std::size_t i = _numMaxFiles; i > 0; i--)
	{
		menuManager.remove("main/file/MRU" + string::to_string(i));
	}

	menuManager.remove("main/file/mruempty");

	if (_list.empty())
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
	
	// Set the iterator to the first filename in the list
	MRUList::iterator i = _list.begin();

	// Add all the created widgets to the menu
	for (MRUMenuItem& item : _menuItems)
	{
		// The default string to be loaded into the widget (i.e. "inactive")
		std::string filename;

		// If the end of the list is reached, do nothing, otherwise increase the iterator
		if (i != _list.end())
		{
			filename = (*i);
			i++;
		}

		if (filename.empty())
		{
			continue;
		}

		std::string label = string::to_string(item.getIndex()) + " - " + filename;

		const std::string commandName = std::string("MRUOpen") + string::to_string(item.getIndex());

		// Create the menu item
		menuManager.insert(
			"main/file/mruseparator",
			"MRU" + string::to_string(item.getIndex()),
			ui::menuItem,
			label,
			"", // empty icon
			commandName
		);

		item.setMapFilename(filename);
	}
}

void MRU::constructMenu()
{
	// Get the menumanager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	// Insert the last separator to split the MRU file list from the "Exit" command.
	menuManager.insert(
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
#endif

void MRU::loadMRUMap(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: LoadMRUMap <index:1..N>" << std::endl;
		return;
	}

	int index = args[0].getInt();

	if (index < 1 || index > _numMaxFiles)
	{
		throw cmd::ExecutionFailure(fmt::format(_("Index out of range: {0:d}"), index));
	}

	// TODO
}

const std::string& MRU::getName() const
{
	static std::string _name(MODULE_MRU_MANAGER);
	return _name;
}

const StringSet& MRU::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void MRU::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	constructPreferences();

	_numMaxFiles = registry::getValue<std::size_t>(RKEY_MRU_LENGTH);
	_list.reset(new MRUList(_numMaxFiles));

	GlobalCommandSystem().addCommand(LOAD_MRU_MAP_CMD,
		std::bind(&MRU::loadMRUMap, this, std::placeholders::_1));

	// Create shortcuts for the items we hold
	for (std::size_t i = 1; i <= _numMaxFiles; i++)
	{
		auto statementName = fmt::format(LOAD_MRU_STATEMENT_FORMAT, i);
		auto statementValue = fmt::format("{0} {0:d}", LOAD_MRU_MAP_CMD, i);

		GlobalCommandSystem().addStatement(statementName, statementValue);
	}

	// Load the most recently used files list from the registry
	loadRecentFiles();
}

module::StaticModule<MRU> mruModule;

} // namespace
