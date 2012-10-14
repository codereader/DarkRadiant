#pragma once

#include <list>
#include "iregistry.h"

#include "MRUMenuItem.h"
#include "MRUList.h"

/* greebo: MRU stands for "Most Recently Used" (maps) and this is what
 * this class is handling.
 *
 * To keep track of the recent filenames the class is using a boost::multi_index
 * container with n elements. The subclass MRUList is providing the interface
 * and iterators for inserting and manipulating the list.
 */
namespace ui
{

class MRU : 
	public sigc::trackable
{
private:
	// The list type containing the menuItem widgets
	typedef std::list<MRUMenuItem> MenuItems;

	// The maximum number files that are remembered (value stored in the registry)
	std::size_t _numMaxFiles;
	bool _loadLastMap;

	// The list of filenames (encapsulated in the helper class MRUList)
	MRUList _list;

	// The list of MRUMenuItems containing the widgets
	MenuItems _menuItems;

	// The empty menuitem (for displaying "Recent files")
	MRUMenuItem _emptyMenuItem;

public:
	// Constructor (initialises the widgets and loads the list from the registry)
	MRU();

	// Inserts the given map filename at the top of the list
	// Duplicates are relocated, the number of list items is constrained
	void insert(const std::string& fileName);

	// Triggers the load of the specified map
	void loadMap(const std::string& fileName);

	// Returns true, if the last map is to be reloaded at startup
	bool loadLastMap() const;

	// Returns the filename of the last opened map, or "" if there doesn't exist one
	std::string getLastMapName();

	// Called on radiant startup
	void initialise();

	// Saves the current list into the Registry.
	void saveRecentFiles();

private:
	// Loads the current filenames into the menu widgets (called after inserts, for example)
	void updateMenu();

	// Add the menu items to the GlobalUIManager
	void constructMenu();

	// Loads the recently used file list from the registry
	void loadRecentFiles();

	// Construct the orthoview preference page and add it to the given group
	void constructPreferences();

	void keyChanged();
};

} // namespace

// The accessor function to the global MRU instance
ui::MRU& GlobalMRU();
