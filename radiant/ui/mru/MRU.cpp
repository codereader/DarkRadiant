#include "MRU.h"

#include "ieventmanager.h"
#include <iostream>
#include "gtk/gtkmenu.h"
#include "gtk/gtkcontainer.h"
#include "string/string.h"
#include "generic/callback.h"
#include "os/file.h"

#include "qe3.h"
#include "map.h"

namespace ui { 

MRU::MRU() :
	_numMaxFiles(GlobalRegistry().getInt(RKEY_MRU_LENGTH)),
	_loadLastMap(GlobalRegistry().get(RKEY_LOAD_LAST_MAP) == "1"),
	_list(_numMaxFiles),
	_emptyMenuItem(RECENT_FILES_CAPTION, *this, 0)
{
	GlobalRegistry().addKeyObserver(this, RKEY_MRU_LENGTH);
	
	// greebo: Register this class in the preference system so that the constructPreferencePage() gets called.
	GlobalPreferenceSystem().addConstructor(this);
	
	// Create _numMaxFiles menu items
	for (unsigned int i = 0; i < _numMaxFiles; i++) {
		
		_menuItems.push_back(MRUMenuItem(intToStr(i), *this, i+1));
		
		MRUMenuItem& item = (*_menuItems.rbegin());
		
		const std::string commandName = std::string("MRUOpen") + intToStr(i);
		
		// Connect the command to the last inserted menuItem
		IEventPtr event = GlobalEventManager().addCommand(commandName, 
					MemberCaller<MRUMenuItem, &MRUMenuItem::activate>(item));
		
		if (event != NULL) {
			event->connectWidget(item);
		}
	}
}

void MRU::loadRecentFiles() {
	// Loads the registry values from the last to the first (recentMap4 ... recentMap1) and
	// inserts them. After everything is loaded, the file list is sorted correctly.
	for (unsigned int i = _numMaxFiles; i > 0; i--) {
		
		const std::string key = RKEY_MAP_MRUS + "/map" + intToStr(i);
		const std::string fileName = GlobalRegistry().get(key);
		
		// Insert the filename
		insert(fileName);
	}
	
	if (_list.empty()) {
		_emptyMenuItem.show();
	}
}

void MRU::saveRecentFiles() {
	// Delete all existing MRU/element nodes
	GlobalRegistry().deleteXPath(RKEY_MAP_MRUS);
	
	unsigned int counter = 1;
	
	// Now wade through the list and save them in the correct order
	for (MRUList::iterator i = _list.begin(); i != _list.end(); counter++, i++) {
		
		const std::string key = RKEY_MAP_MRUS + "/map" + intToStr(counter);
		
		// Save the string into the registry
		GlobalRegistry().set(key, (*i));
	}
}

void MRU::loadMap(const std::string& fileName) {
	if (ConfirmModified("Open Map")) {
		if (file_readable(fileName.c_str())) {
			// Shut down the current map
			Map_Free();
			
			// Load the file
			Map_LoadFile(fileName.c_str());
			
			// Update the MRU list with this file
			insert(fileName);
		}
	}
}

void MRU::keyChanged() {
	// greebo: Don't load the new number of maximum files from the registry, 
	// this would mess up the existing widgets, wait for the DarkRadiant restart instead
	//_numMaxFiles = GlobalRegistry().getInt(RKEY_MRU_LENGTH);
	_loadLastMap = (GlobalRegistry().get(RKEY_LOAD_LAST_MAP) == "1");
}

// Construct the orthoview preference page and add it to the given group
void MRU::constructPreferencePage(PreferenceGroup& group) {
	PreferencesPage* page(group.createPage("Map Files", "Map File Preferences"));
	
	page->appendEntry("Number of most recently used files", RKEY_MRU_LENGTH);
	page->appendCheckBox("", "Open last map on startup", RKEY_LOAD_LAST_MAP);
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
	// Set the iterator to the first item in the list
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

WidgetList MRU::getMenuWidgets() {
	WidgetList widgetList;

	widgetList.push_back(_emptyMenuItem);
	//gtk_widget_show(_emptyMenuItem);

	// Add all the created widgets to the menu
	for (MenuItems::iterator m = _menuItems.begin(); m != _menuItems.end(); m++) {
		MRUMenuItem& item = (*m);
		
		// greebo: Add the items to the list 
		// It will be shown/hidden by the load() and insert() methods 
		widgetList.push_back(item);
	}

	// Call the update routine to load the values into the widgets
	updateMenu();
	
	return widgetList;
}

// -------------------------------------------------------------------------------------

} // namespace ui

// The accessor function to the MRU instance
ui::MRU& GlobalMRU() {
	static ui::MRU _mruInstance;
	return _mruInstance;
}
