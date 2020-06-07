#pragma once

#include <list>
#include "iregistry.h"
#include "imru.h"

#include "MRUList.h"

/* greebo: MRU stands for "Most Recently Used" (maps) and this is what
 * this class is handling.
 *
 * To keep track of the recent filenames the class is using a container 
 * with n elements. The subclass MRUList is providing the interface
 * and iterators for inserting and manipulating the list.
 */
namespace map
{

class MRU : 
	public IMRUManager
{
private:
#if 0
	// The list type containing the menuItem widgets
	typedef std::list<MRUMenuItem> MenuItems;
#endif
	// The maximum number files that are remembered (value stored in the registry)
	std::size_t _numMaxFiles;

	// The list of filenames (encapsulated in the helper class MRUList)
	std::unique_ptr<MRUList> _list;

	sigc::signal<void> _signalMapListChanged;

#if 0
	// The list of MRUMenuItems containing the widgets
	MenuItems _menuItems;

	// The empty menuitem (for displaying "Recent files")
	MRUMenuItem _emptyMenuItem;
#endif
public:
	std::size_t getMaxNumberOfItems() const override;

	void foreachItem(const ItemFunctor& functor) override;

	// Inserts the given map filename at the top of the list
	// Duplicates are relocated, the number of list items is constrained
	void insert(const std::string& fileName) override;

	// Triggers the load of the specified map
	void loadMap(const std::string& fileName);

	// Returns the filename of the last opened map, or "" if there doesn't exist one
	std::string getLastMapName() override;

	sigc::signal<void>& signal_MapListChanged() override;

	// Saves the current list into the Registry.
	void saveRecentFiles();

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
#if 0
	// Loads the current filenames into the menu widgets (called after inserts, for example)
	void updateMenu();

	// Add the menu items to the GlobalUIManager
	void constructMenu();
#endif
	// Loads the recently used file list from the registry
	void loadRecentFiles();

	// Construct the orthoview preference page and add it to the given group
	void constructPreferences();

	void loadMRUMap(const cmd::ArgumentList& args);
};

} // namespace
