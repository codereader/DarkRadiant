#ifndef MRU_H_
#define MRU_H_

#include <list>
#include "iregistry.h"

#include "MRUMenuItem.h"
#include "MRUList.h"

// Forward declaration
typedef struct _GtkWidget GtkWidget;

/* greebo: MRU stands for "Most Recently Used" (maps) and this is what 
 * this class is handling.
 * 
 * To keep track of the recent filenames the class is using a boost::multi_index
 * container with n elements. The subclass MRUList is providing the interface
 * and iterators for inserting and manipulating the list. 
 */
namespace ui { 

	namespace {
		const std::string RKEY_MAP_ROOT = "user/ui/map";
		const std::string RKEY_MAP_MRUS = RKEY_MAP_ROOT + "/MRU";
		const std::string RKEY_MRU_LENGTH = RKEY_MAP_ROOT + "/numMRU";
		const std::string RKEY_LOAD_LAST_MAP = RKEY_MAP_ROOT + "/loadLastMap";
		
		const std::string RECENT_FILES_CAPTION = "Recently used Maps";
	}

class MRU :
	public RegistryKeyObserver
{
	// The list type containing the menuItem widgets
	typedef std::list<MRUMenuItem> MenuItems;
	
	// The maximum number files that are remembered (value stored in the registry)
	unsigned int _numMaxFiles;
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
	
	// Loads the recently used file list from the registry
	void loadRecentFiles();
	
	// Inserts the given map filename at the top of the list
	// Duplicates are relocated, the number of list items is constrained
	void insert(const std::string& fileName);
	
	// Triggers the load of the specified map 
	void loadMap(const std::string& fileName);
	
	// Saves the current list into the Registry.
	void saveRecentFiles();
	
	// Returns true, if the last map is to be reloaded at startup
	bool loadLastMap() const;
	
	// Returns the filename of the last opened map, or "" if there doesn't exist one
	std::string getLastMapName();
	
	// The callback for registry key changes
	void keyChanged(const std::string& key, const std::string& val);
	
	// Add the menu items to the GlobalUIManager 
	void constructMenu();
	
	// Construct the orthoview preference page and add it to the given group
	void constructPreferences();

private:
	// Loads the current filenames into the menu widgets (called after inserts, for example) 
	void updateMenu();
	
}; // class MRU

} // namespace ui

// The accessor function to the global MRU instance
ui::MRU& GlobalMRU();

#endif /*MRU_H_*/
