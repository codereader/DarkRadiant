#include "MRU.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "ipreferencesystem.h"
#include "iuimanager.h"
#include "string/string.h"
#include "os/file.h"
#include "registry/registry.h"
#include "gtkutil/dialog/MessageBox.h"

#include "map/Map.h"

namespace ui {

	namespace {
		const std::string RKEY_MAP_ROOT = "user/ui/map";
		const std::string RKEY_MAP_MRUS = RKEY_MAP_ROOT + "/MRU";
		const std::string RKEY_MRU_LENGTH = RKEY_MAP_ROOT + "/numMRU";
		const std::string RKEY_LOAD_LAST_MAP = RKEY_MAP_ROOT + "/loadLastMap";

		const char* const RECENT_FILES_CAPTION = N_("Recently used Maps");
	}

MRU::MRU() :
	_numMaxFiles(registry::getValue<int>(RKEY_MRU_LENGTH)),
	_loadLastMap(registry::getValue<bool>(RKEY_LOAD_LAST_MAP)),
	_list(_numMaxFiles),
	_emptyMenuItem(_(RECENT_FILES_CAPTION), *this, 0)
{
	GlobalRegistry().signalForKey(RKEY_MRU_LENGTH).connect(
        sigc::mem_fun(this, &MRU::keyChanged)
    );

	// Add the preference settings
	constructPreferences();

	// Create _numMaxFiles menu items
	for (unsigned int i = 0; i < _numMaxFiles; i++) {

		_menuItems.push_back(MRUMenuItem(string::to_string(i), *this, i+1));

		MRUMenuItem& item = (*_menuItems.rbegin());

		const std::string commandName = std::string("MRUOpen") + string::to_string(i+1);

		// Connect the command to the last inserted menuItem
		GlobalCommandSystem().addCommand(
			commandName,
			boost::bind(&MRUMenuItem::activate, &item, _1)
		);
		GlobalEventManager().addCommand(commandName, commandName);
	}
}

void MRU::loadRecentFiles()
{
	// Loads the registry values from the last to the first (recentMap4 ... recentMap1) and
	// inserts them. After everything is loaded, the file list is sorted correctly.
	for (std::size_t i = _numMaxFiles; i > 0; i--)
	{
		const std::string key = RKEY_MAP_MRUS + "/map" + string::to_string(i);
		const std::string fileName = GlobalRegistry().get(key);

		// Insert the filename
		insert(fileName);
	}

	if (_list.empty())
	{
		_emptyMenuItem.show();
	}
}

void MRU::saveRecentFiles()
{
	// Delete all existing MRU/element nodes
	GlobalRegistry().deleteXPath(RKEY_MAP_MRUS);

	std::size_t counter = 1;

	// Now wade through the list and save them in the correct order
	for (MRUList::const_iterator i = _list.begin(); i != _list.end(); ++counter, ++i)
	{
		const std::string key = RKEY_MAP_MRUS + "/map" + string::to_string(counter);

		// Save the string into the registry
		GlobalRegistry().set(key, (*i));
	}
}

void MRU::loadMap(const std::string& fileName)
{
	if (GlobalMap().askForSave(_("Open Map")))
	{
		if (file_readable(fileName.c_str()))
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
			gtkutil::MessageBox::ShowError(
				(boost::format(_("Could not read map file: %s")) % fileName).str(),
				GlobalMainFrame().getTopLevelWindow());
		}
	}
}

void MRU::keyChanged()
{
	// greebo: Don't load the new number of maximum files from the registry,
	// this would mess up the existing widgets, wait for the DarkRadiant restart instead
	//_numMaxFiles = registry::getValue<int>(RKEY_MRU_LENGTH);
	_loadLastMap = registry::getValue<bool>(RKEY_LOAD_LAST_MAP);
}

// Construct the MRU preference page and add it to the given group
void MRU::constructPreferences()
{
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Map Files"));

	page->appendEntry(_("Number of most recently used files"), RKEY_MRU_LENGTH);
	page->appendCheckBox("", _("Open last map on startup"), RKEY_LOAD_LAST_MAP);
}

void MRU::initialise()
{
	// Construct the MRU commands and menu structure
	constructMenu();

	// Initialise the most recently used files list
	loadRecentFiles();
}

bool MRU::loadLastMap() const {
	return _loadLastMap;
}

std::string MRU::getLastMapName() {
	if (_list.empty()) {
		return "";
	}
	else {
		MRUList::iterator i = _list.begin();
		return *i;
	}
}

void MRU::insert(const std::string& fileName) {

	if (fileName != "") {
		// Insert the item into the filelist
		_list.insert(fileName);

		// Hide the empty menu item, as we're having a MRU map now
		_emptyMenuItem.hide();

		// Update the widgets
		updateMenu();
	}
}

void MRU::updateMenu() {
	// Set the iterator to the first filename in the list
	MRUList::iterator i = _list.begin();

	// Now cycle through the widgets and load the values
	for (MenuItems::iterator m = _menuItems.begin(); m != _menuItems.end(); m++) {

		// The default string to be loaded into the widget (i.e. "inactive")
		std::string fileName = "";

		// If the end of the list is reached, do nothing, otherwise increase the iterator
		if (i != _list.end()) {
			fileName = (*i);
			i++;
		}

		// Set the label (widget is shown/hidden automatically)
		m->setLabel(fileName);
	}
}

void MRU::constructMenu() {
	// Get the menumanager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	// Create the "empty" MRU menu item (the desensitised one)
	Gtk::Widget* empty = menuManager.insert(
		"main/file/exit",
		"mruempty",
		ui::menuItem,
		RECENT_FILES_CAPTION,
		"", // empty icon
		"" // empty event
	);

	_emptyMenuItem.setWidget(empty);
	empty->hide();

	// Add all the created widgets to the menu
	for (MenuItems::iterator m = _menuItems.begin(); m != _menuItems.end(); ++m)
	{
		MRUMenuItem& item = (*m);

		const std::string commandName = std::string("MRUOpen") + string::to_string(item.getIndex());

		// Create the toplevel menu item
		Gtk::Widget* menuItem = menuManager.insert(
			"main/file/exit",
			"MRU" + string::to_string(item.getIndex()),
			ui::menuItem,
			item.getLabel(),
			"", // empty icon
			commandName
		);

		item.setWidget(menuItem);
	}

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

// -------------------------------------------------------------------------------------

} // namespace ui

// The accessor function to the MRU instance
ui::MRU& GlobalMRU() {
	static ui::MRU _mruInstance;
	return _mruInstance;
}
